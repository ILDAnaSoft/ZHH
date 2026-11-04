"""Runs the cutflow of config/mmHHbbbb.yaml on dummy data, including a minimal MVA training
(see dummy_cutflow_data.py). The test succeeds only if all expected output files exist."""

from pathlib import Path

import pytest
import yaml

from .conftest import REPO_ROOT, SoftwareEnvironment, describe
from .dummy_cutflow_data import N_EVENTS_PER_SOURCE, expected_outputs

pytestmark = pytest.mark.integration

STEERING_FILE = REPO_ROOT / 'config' / 'mmHHbbbb.yaml'

@pytest.fixture(scope='module')
def dummy_data(software_environment:SoftwareEnvironment, tmp_path_factory)->Path:
    """Directory with the ROOT files and the steering file"""
    output_dir = tmp_path_factory.mktemp('cutflow_dummy_data')
    script = Path(__file__).with_name('dummy_cutflow_data.py')

    result = software_environment.run(f'python {script} {STEERING_FILE} {output_dir}', cwd=output_dir, timeout=900)
    assert result.returncode == 0, f'Could not create the dummy data: {describe(result)}'

    return output_dir

def test_dummy_data_has_all_branches(dummy_data:Path):
    import uproot

    with open(STEERING_FILE, encoding='utf-8') as f:
        steer = yaml.safe_load(f)

    for source in steer['sources']:
        with uproot.open(dummy_data / 'root' / source['name'] / f'{source["name"]}.root') as root_file:
            for feature in steer['features']['interpret']:
                if 'tree' not in feature:
                    continue

                tree = root_file[feature['tree']]
                assert tree.num_entries == N_EVENTS_PER_SOURCE

                for name in (feature['names'] if 'names' in feature else [feature['name']]):
                    branch = feature.get('branch', name)
                    assert branch in tree, f'{branch} is missing in {feature["tree"]} of source {source["name"]}'

def test_cutflow(software_environment:SoftwareEnvironment, dummy_data:Path, tmp_path:Path):
    steering_file = dummy_data / STEERING_FILE.name

    result = software_environment.run(f'cutflow {steering_file}', cwd=tmp_path, timeout=3600)
    output = f'{describe(result)}\nFiles found: {sorted(path.name for path in tmp_path.iterdir())}'
    assert result.returncode == 0, f'cutflow failed: {output}'

    with open(steering_file, encoding='utf-8') as f:
        expected = expected_outputs(yaml.safe_load(f))

    missing = [file for file in expected if not (tmp_path / file).is_file() or not (tmp_path / file).stat().st_size]
    assert not missing, f'cutflow did not write {missing}: {output}'
