"""Runs the steering files of the reconstruction and the analysis one after another with Marlin directly,
i.e. as a user would do with the function MarlinZHH of setup.sh, on one of the files of the sample."""

import shlex
from pathlib import Path

import pytest

from .conftest import N_EVENTS, REPO_ROOT, SoftwareEnvironment, describe

pytestmark = pytest.mark.integration

RECO_OUTPUT = 'zhh_reco.slcio'
ANALYSIS_OUTPUT = 'zhh_AIDA.root'

# as in the configurations of the fast simulation
CONSTANTS = '--constant.CMSEnergy=550 --constant.errorflowconfusion=False'

def marlin(env:SoftwareEnvironment, steering_file:str, input_file:Path|str, cwd:Path):
    """The exit code of Marlin is not meaningful: it may abort in static destructors after finishing
    all events. As in the law tasks, the result is judged by the output files"""
    command = (f'MarlinZHH {REPO_ROOT}/scripts/{steering_file} {CONSTANTS} --global.MaxRecordNumber=0 '
               f'--global.LCIOInputFiles={shlex.quote(str(input_file))}')

    return env.run(command, cwd=cwd, timeout=3600)

@pytest.fixture(scope='module')
def workdir(tmp_path_factory)->Path:
    return tmp_path_factory.mktemp('marlin')

@pytest.fixture(scope='module')
def reconstruction(software_environment:SoftwareEnvironment, sample:list[Path], workdir:Path)->Path:
    result = marlin(software_environment, 'prod_reco_run.xml', sample[0], workdir)
    output = workdir / RECO_OUTPUT
    assert output.is_file(), f'prod_reco_run.xml did not write {RECO_OUTPUT}: {describe(result)}'

    return output

@pytest.fixture(scope='module')
def analysis(software_environment:SoftwareEnvironment, reconstruction:Path, workdir:Path)->Path:
    result = marlin(software_environment, 'prod_analysis_run.xml', reconstruction, workdir)
    output = workdir / ANALYSIS_OUTPUT
    assert output.is_file(), f'prod_analysis_run.xml did not write {ANALYSIS_OUTPUT}: {describe(result)}'

    return output

def test_reconstruction(software_environment:SoftwareEnvironment, reconstruction:Path):
    result = software_environment.run(f'lcio_event_counter {reconstruction}', timeout=300)
    assert result.returncode == 0, describe(result)
    assert 0 < int(result.stdout.strip().splitlines()[-1]) <= N_EVENTS

@pytest.mark.parametrize('tree', ['FinalStates', 'EventObservablesLL', 'EventObservablesVV', 'EventObservablesQQ'])
def test_analysis(analysis:Path, tree:str):
    import uproot

    with uproot.open(analysis) as root_file:
        assert tree in root_file, f'{tree} is missing in {analysis}'
        assert 0 < root_file[tree].num_entries <= N_EVENTS
