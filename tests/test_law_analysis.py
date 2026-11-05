"""Runs AnalysisFinal, including all the tasks it depends on, as local workflows. The tasks up to
FastSimSGV must not run again: their outputs (see shell/ci_prepare_sample.sh) are reused."""

from pathlib import Path

import pytest

from .conftest import N_EVENTS, SoftwareEnvironment, describe, law_succeeded

pytestmark = pytest.mark.integration

# all tasks after FastSimSGV producing files in their data directory
TASKS = ['RawIndex', 'RecoRuntime', 'CreateRecoChunks', 'RecoFinal', 'AnalysisIndex', 'AnalysisRuntime', 'CreateAnalysisChunks', 'AnalysisFinal']

@pytest.fixture(scope='module')
def analysis_final(software_environment:SoftwareEnvironment, sample:list[Path]):
    generated = sorted(software_environment.data_dir('WhizardEventGeneration').glob('*.slcio')) + sample
    assert generated, 'no events found'

    before = { path: path.stat().st_mtime_ns for path in generated }
    result = software_environment.law('AnalysisFinal', timeout=4 * 3600)

    return software_environment, result, before

def test_chain_succeeds(analysis_final):
    _, result, _ = analysis_final
    assert law_succeeded(result), f'law run AnalysisFinal failed: {describe(result)}'

def test_sample_is_reused(analysis_final):
    _, _, before = analysis_final
    assert before == { path: path.stat().st_mtime_ns for path in before }

@pytest.mark.parametrize('task', TASKS)
def test_task_has_outputs(analysis_final, task:str):
    env, _, _ = analysis_final
    assert any(env.data_dir(task).iterdir()), f'{task} did not write anything to {env.data_dir(task)}'

def test_analysis_output_has_events(analysis_final):
    import uproot

    env, _, _ = analysis_final
    files = sorted(env.data_dir('AnalysisFinal').glob('*.root'))
    assert files, f'AnalysisFinal did not write any ROOT files to {env.data_dir("AnalysisFinal")}'

    n_events = 0
    for file in files:
        with uproot.open(file) as root_file:
            n_events += root_file['FinalStates'].num_entries
            for tree in ['EventObservablesLL', 'EventObservablesVV', 'EventObservablesQQ']:
                assert tree in root_file, f'{tree} is missing in {file}'

    assert n_events > N_EVENTS
