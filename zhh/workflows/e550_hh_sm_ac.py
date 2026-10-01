"""Kappa_lambda (Higgs trilinear self-coupling) scan for the neutrino-pair
double-Higgs processes n1n1hh (e+e- -> nu_e nu_e~ H H) and n23n23hh
(e+e- -> nu_mu/tau nu_mu/tau~ H H) at 550 GeV, using the SM_ac_CKM
anomalous-coupling model with fac_gh3 = kappa_lambda.

Only eL.pR and eR.pL are generated: n1n1hh/n23n23hh receive contributions
from both the WW-fusion diagrams (require a left-handed e-/right-handed
e+, i.e. eL.pR, since the charged current is purely left-handed) and the
ZHH-with-invisible-Z diagrams (present for either beam helicity
combination); eL.pL/eR.pR do not contribute.

Uses the dedicated steering template workflows/resources/whizard_template/
whizard.base550.hh.sin, NOT the SM-background whizard.base550.sin. The HH
template leaves ?resonance_history off (otherwise WHIZARD 3.x writes the
on-shell intermediate Z/W of the signal diagrams into the MCParticle record,
unlike the Whizard 2.8.5 reference samples E550-Test.P{n1n1hh,n23n23hh})
and does not set $restrictions = "!H" (the s-channel H* -> H H diagram
carries the whole fac_gh3 dependence this scan measures). Verified against
the reference: n1n1hh sigma 0.0408 fb vs 0.0407 fb, n23n23hh 0.0500 fb vs
0.0501 fb, flat  e+ e- -> nu nu-bar h h  record in both.

marlin_task_kwargs and marlin_constants below mirror workflows/analysis/
configs/conf_550_fast_base.py's Config_550_base (steering_file, expected
output filenames zhh_reco.slcio/zhh_reco_FinalStateMeta.json/zhh_AIDA.root,
and ZHH_REPO_ROOT for the LCFIPlus ONNX model paths in prod_reco_run.xml)
rather than reinventing untested equivalents, since that pattern is already
proven against the same steering files in production.

Registered via zhh/workflows/plugin.py's register() (the "hep_workflows.tasks"
entry point loaded automatically whenever hep_workflows is imported - see
hep_workflows/load_plugins.py), so no --module flag is needed, e.g.:

    law run WhizardEventGeneration --tag=550-vvhh-kl1.0-fast-perf
"""

from os import environ

from hep_workflows.framework import AnalysisConfiguration, configurations
from hep_workflows.tasks_sim import FastSimSGV
from hep_workflows.utils.types import WhizardOption, SGVOptions


TEMPLATE_DIR = '$REPO_ROOT/workflows/resources/whizard_template'
SINDARIN_FILE = 'whizard.base550.hh.sin'

# n1n1hh/n23n23hh only receive contributions from eL.pR and eR.pL (see module
# docstring); 10 iterations x 100k events = 1M events per process per
# polarization.
CONTRIBUTING_POLARIZATIONS = {'eL.pR': 10, 'eR.pL': 10}
EVENTS_PER_ITERATION = 100_000


def _whizard_options(kappa_lambda: float) -> list[WhizardOption]:
    return [
        {
            'process_name': 'n1n1hh',
            'process_definition': (
                'model = SM_ac_CKM\n'
                'fghgaga = 0\n'
                'fghgaz = 0\n'
                f'fac_gh3 = {kappa_lambda}\n'
                'process n1n1hh = e1,E1 => n1, N1, h, h { $omega_flags = "-model:constant_width" }'
            ),
            'template_dir': TEMPLATE_DIR,
            'sindarin_file': SINDARIN_FILE,
            'iters_per_polarization': dict(CONTRIBUTING_POLARIZATIONS),
            'nevents': EVENTS_PER_ITERATION,
        },
        {
            'process_name': 'n23n23hh',
            'process_definition': (
                'model = SM_ac_CKM\n'
                'fghgaga = 0\n'
                'fghgaz = 0\n'
                f'fac_gh3 = {kappa_lambda}\n'
                'alias not_nu_e = n2:n3:N2:N3\n'
                'process n23n23hh = e1,E1 => not_nu_e, not_nu_e, h, h { $omega_flags = "-model:constant_width" }'
            ),
            'template_dir': TEMPLATE_DIR,
            'sindarin_file': SINDARIN_FILE,
            'iters_per_polarization': dict(CONTRIBUTING_POLARIZATIONS),
            'nevents': EVENTS_PER_ITERATION,
        },
    ]


def _sgv_inputs(self, fast_sim_task) -> tuple[list[str], list[SGVOptions]]:
    assert isinstance(fast_sim_task, FastSimSGV)

    inputs = fast_sim_task.input()
    assert 'whizard_event_generation' in inputs

    whiz_outputs = inputs['whizard_event_generation']['collection']
    input_files = [whiz_outputs[i][0].path for i in range(len(whiz_outputs))]
    input_options: list[SGVOptions] = [{
        'global_steering.MAXEV': 999999,
        'global_generation_steering.CMS_ENE': 550,
        'external_read_generation_steering.GENERATOR_INPUT_TYPE': 'LCIO',
        'external_read_generation_steering.INPUT_FILENAMES': 'input.slcio',
        'analysis_steering.CALO_TREATMENT': 'PERF',
    }] * len(input_files)

    return input_files, input_options


def _marlin_task_kwargs(task) -> dict:
    """Mirrors Config_550_base.marlin_task_kwargs_factory in
    workflows/analysis/configs/conf_550_fast_base.py."""

    is_reco = task.__class__.__name__.lower().startswith('reco')

    return {
        'check_output_root_ttrees': None if is_reco else [
            ('zhh_AIDA.root', 'EventObservablesLL'),
            ('zhh_AIDA.root', 'EventObservablesVV'),
            ('zhh_AIDA.root', 'FinalStates'),
            ('zhh_AIDA.root', 'KinFitLL_ZHH'),
            ('zhh_AIDA.root', 'KinFitLL_ZZH'),
            ('zhh_AIDA.root', 'KinFitVV_ZHH'),
            ('zhh_AIDA.root', 'KinFitVV_ZZH'),
        ],
        'check_output_files_exist': ['zhh_reco_FinalStateMeta.json'] if is_reco else [],
        'check_output_lcio_files': ['zhh_reco.slcio'] if is_reco else None,
        'output_file': 'zhh_reco.slcio' if is_reco else 'zhh_AIDA.root',
        'steering_file': f'{environ["REPO_ROOT"]}/scripts/prod_reco_run.xml' if is_reco
            else f'{environ["REPO_ROOT"]}/scripts/prod_analysis_run.xml',
    }


class _E550VVHHKlBaseConfig(AnalysisConfiguration):
    sqrt_s = 550.
    sgv_inputs = _sgv_inputs
    task_kwargs = {'MarlinBaseJob': _marlin_task_kwargs}
    marlin_globals = {}
    marlin_constants = {'CMSEnergy': 550, 'errorflowconfusion': 'False', 'ZHH_REPO_ROOT': environ['REPO_ROOT']}


class E550VVHHKlM5FastPerfConfig(_E550VVHHKlBaseConfig):
    tag = '550-vvhh-kl-5-fast-perf'
    whizard_options = _whizard_options(-5)


class E550VVHHKlM2FastPerfConfig(_E550VVHHKlBaseConfig):
    tag = '550-vvhh-kl-2-fast-perf'
    whizard_options = _whizard_options(-2)


class E550VVHHKl0p5FastPerfConfig(_E550VVHHKlBaseConfig):
    tag = '550-vvhh-kl0.5-fast-perf'
    whizard_options = _whizard_options(0.5)


class E550VVHHKl1p0FastPerfConfig(_E550VVHHKlBaseConfig):
    tag = '550-vvhh-kl1.0-fast-perf'
    whizard_options = _whizard_options(1.0)


class E550VVHHKl1p5FastPerfConfig(_E550VVHHKlBaseConfig):
    tag = '550-vvhh-kl1.5-fast-perf'
    whizard_options = _whizard_options(1.5)


class E550VVHHKl4FastPerfConfig(_E550VVHHKlBaseConfig):
    tag = '550-vvhh-kl4-fast-perf'
    whizard_options = _whizard_options(4)


class E550VVHHKl7FastPerfConfig(_E550VVHHKlBaseConfig):
    tag = '550-vvhh-kl7-fast-perf'
    whizard_options = _whizard_options(7)


for _cfg_cls in (
    E550VVHHKlM5FastPerfConfig,
    E550VVHHKlM2FastPerfConfig,
    E550VVHHKl0p5FastPerfConfig,
    E550VVHHKl1p0FastPerfConfig,
    E550VVHHKl1p5FastPerfConfig,
    E550VVHHKl4FastPerfConfig,
    E550VVHHKl7FastPerfConfig,
):
    configurations.add(_cfg_cls())
