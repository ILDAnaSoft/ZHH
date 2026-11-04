"""Fixtures for the integration tests: they run Marlin, law and cutflow in subprocesses after
sourcing workflows/setup.sh, i.e. with key4hep from CVMFS and the Python virtual environment.

All tests use a small sample of the configuration TAG. In the Docker image it has already been
produced at build time (see shell/ci_prepare_sample.sh). It is found in $ZHH_TEST_DATA_PATH, which
replaces DATA_PATH, as the law tasks are complete then. Without $ZHH_TEST_DATA_PATH, a temporary
directory is used, so that no existing data is touched, and the sample is produced on first use.
"""

import os
import shlex
import subprocess
from pathlib import Path

import pytest

REPO_ROOT = Path(__file__).resolve().parents[1]
SETUP_SH = REPO_ROOT / 'workflows' / 'setup.sh'
CVMFS_KEY4HEP = Path('/cvmfs/sw.hsf.org/key4hep')

TAG = '550-llbb-fast-perf'
N_EVENTS = 100

# one file per beam polarization, see WhizardSteeringFileConstructor
N_SAMPLE_FILES = 4

def pytest_collection_modifyitems(config, items):
    if CVMFS_KEY4HEP.is_dir() and (REPO_ROOT / 'setup.sh').is_file():
        return

    skip = pytest.mark.skip(reason=f'{CVMFS_KEY4HEP} or {REPO_ROOT}/setup.sh not found: run install.sh on a machine with CVMFS')
    for item in items:
        item.add_marker(skip)

class SoftwareEnvironment:
    """Runs commands in the environment of workflows/setup.sh"""

    def __init__(self, workdir:Path, data_path:Path):
        self.workdir = workdir
        self.data_path = data_path
        self.process_env = { **os.environ,
                             # run the whole dependency tree locally, not on HTCondor (see BaseWorkflowTask of hep_workflows)
                             'HEP_WORKFLOWS_DEFAULT_WORKFLOW': 'local',
                             'DEBUG_NEVENTS': str(N_EVENTS),
                             # the tasks run in parallel; on a batch system they get one core each
                             'OMP_NUM_THREADS': '1' }

    def run(self, command:str, cwd:Path|None=None, timeout:float|None=None)->subprocess.CompletedProcess:
        """Runs a shell command after sourcing setup.sh. DATA_PATH must be set afterwards, as .env overwrites it.
        Some of the software writes files (e.g. cdb.log) to the working directory, which is why it defaults to
        a temporary directory. stdin is closed so that a prompt fails instead of blocking until the timeout."""
        setup = f'source {shlex.quote(str(SETUP_SH))} > /dev/null || exit 99; export DATA_PATH={shlex.quote(str(self.data_path))}'

        return subprocess.run(['bash', '-c', f'{setup}; {command}'], cwd=cwd or self.workdir, env=self.process_env,
                              stdin=subprocess.DEVNULL, capture_output=True, text=True, timeout=timeout)

    def law(self, task:str, timeout:float|None=None)->subprocess.CompletedProcess:
        return self.run(shlex.join(['law', 'run', task, f'--tag={TAG}', f'--workers={os.cpu_count()}']), timeout=timeout)

    def data_dir(self, task:str)->Path:
        return self.data_path / task / TAG

def describe(result:subprocess.CompletedProcess)->str:
    """The part of the output of a failed command to show in assertion messages"""
    return f'exit code {result.returncode}\n{(result.stdout + result.stderr)[-6000:]}'

def law_succeeded(result:subprocess.CompletedProcess)->bool:
    return result.returncode == 0 and 'This progress looks :)' in result.stdout + result.stderr

@pytest.fixture(scope='session')
def software_environment(tmp_path_factory)->SoftwareEnvironment:
    workdir = tmp_path_factory.mktemp('zhh')
    configured = os.environ.get('ZHH_TEST_DATA_PATH')
    data_path = Path(configured) if configured else workdir / 'data'
    data_path.mkdir(parents=True, exist_ok=True)

    env = SoftwareEnvironment(workdir, data_path)

    # setup.sh must find key4hep and the Python environment
    result = env.run('which law Marlin whizard cutflow', timeout=600)
    assert result.returncode == 0, f'Could not set up the software environment: {describe(result)}'

    return env

@pytest.fixture(scope='session')
def sample(software_environment:SoftwareEnvironment)->list[Path]:
    """The simulated events. Does nothing if the outputs of FastSimSGV exist, as in the image"""
    result = software_environment.law('FastSimSGV', timeout=4 * 3600)
    assert law_succeeded(result), f'law run FastSimSGV failed: {describe(result)}'

    files = sorted(software_environment.data_dir('FastSimSGV').glob('*.slcio'))
    assert len(files) == N_SAMPLE_FILES, f'expected {N_SAMPLE_FILES} files from FastSimSGV, found {files}'

    return files
