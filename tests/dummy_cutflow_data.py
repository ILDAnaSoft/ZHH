"""Creates everything needed to run the cutflow of a steering file without any simulated data:
a ROOT file per source with normally distributed values for every feature that the steering file
requests, and a copy of the steering file reading these files and running only a minimal MVA training.

The branches are derived from the steering file (features/interpret) and written with ROOT, so that
they have the same layout as the output of AnalysisFinal (e.g. map<int,int> and LorentzVector).

Usage: python dummy_cutflow_data.py <steering file> <output directory>
"""

import argparse
import copy
import json
import os
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np
import yaml

N_EVENTS_PER_SOURCE = 10_000
SEED = 42

# keys of the final_state_counts map in the order of zhh.analysis.PreselectionAnalysis.fs_columns
FSC_KEYS = { 'Nd': 1, 'Nu': 2, 'Ns': 3, 'Nc': 4, 'Nb': 5, 'Nt': 6, 'Ne1': 11, 'Nv1': 12, 'Ne2': 13, 'Nv2': 14,
             'Ne3': 15, 'Nv3': 16, 'Ng': 21, 'Ny': 22, 'NZ': 23, 'NW': 24, 'NH': 25 }

POLARIZATION_CODES = [1, 2] # eL.pR and eR.pL

# values of features without a preselection cut: normal distributions around SHIFT_MEAN + shift * SHIFT_WIDTH
# the shift of each component lets the MVA separate them
SHIFT_MEAN = 50.
SHIFT_WIDTH = 5.

@dataclass
class Component:
    """Events of a physics process with the same final state"""
    process: str
    final_state: dict[str, int]
    shift: float
    cross_section: float # fb
    n_b_from_higgs: int = 0
    # relative number of events within the source
    fraction: float = field(default=1.)

# Each source has to contain all event categories of its event_categorization that the steering file requires,
# i.e. the classes of the MVA (which all need events in both the training and the test dataset) and the entries of
# the cutflow table. The actions plotting the MVA outputs require at least four final states in signal and background.
SOURCES = {
    '4fsl': [
        Component('f4_zz_sl', { 'Ne2': 2, 'Nb': 2 }, shift=-3., cross_section=30.),
        Component('f4_ww_sl', { 'Ne1': 1, 'Nv1': 1, 'Nu': 1, 'Nd': 1 }, shift=-2., cross_section=100.),
        Component('f4_zz_sl', { 'Nv2': 2, 'Nu': 2 }, shift=-2., cross_section=10.)
    ],
    '4flh': [
        Component('f4_zz_l', { 'Ne1': 2, 'Ne2': 2 }, shift=-4., cross_section=20.),
        Component('f4_zz_h', { 'Nu': 2, 'Nd': 2 }, shift=-1., cross_section=100.),
        Component('f4_zz_h', { 'Nb': 4 }, shift=-1., cross_section=10.)
    ],
    '2l4q': [
        Component('f6_eexyyx', { 'Ne2': 2, 'Nu': 2, 'Nd': 2 }, shift=-2., cross_section=2.),
        Component('f6_eexyyx', { 'Ne2': 2, 'Nb': 4 }, shift=-1., cross_section=.2),
        Component('f6_yyvlyx', { 'Ne2': 1, 'Nv2': 1, 'Nb': 2, 'Nu': 1, 'Nd': 1 }, shift=-3., cross_section=50.),
        Component('f6_yyvlyx', { 'Ne1': 1, 'Nv1': 1, 'Nu': 2, 'Nd': 2 }, shift=-3., cross_section=50.)
    ],
    'llhh': [
        Component('e2e2hh', { 'Ne2': 2, 'Nb': 4 }, shift=4., cross_section=.05, n_b_from_higgs=4, fraction=2.),
        Component('e1e1hh', { 'Ne1': 2, 'Nb': 2, 'Nc': 2 }, shift=2., cross_section=.02, n_b_from_higgs=2),
        Component('e2e2hh', { 'Ne2': 2, 'Nb': 2, 'Nc': 2 }, shift=2., cross_section=.02, n_b_from_higgs=2),
        Component('e3e3hh', { 'Ne3': 2, 'Nb': 2, 'Nc': 2 }, shift=2., cross_section=.02, n_b_from_higgs=2),
        Component('e2e2hh', { 'Ne2': 2, 'Nc': 4 }, shift=2., cross_section=.01),
        Component('e2e2qqh', { 'Ne2': 2, 'Nu': 2, 'Nb': 2 }, shift=0., cross_section=.5, n_b_from_higgs=2, fraction=2.)
    ],
    '6q': [
        Component('f6_xxxxxx', { 'Nu': 2, 'Nd': 2, 'Nb': 2 }, shift=-2., cross_section=50.),
        Component('f6_xxxxxx', { 'Nu': 3, 'Nd': 3 }, shift=-2., cross_section=50.)
    ],
    'tthz': [
        Component('f8_tth', { 'Nb': 4, 'Nu': 2, 'Nd': 2 }, shift=-1., cross_section=.5),
        Component('f8_ttz', { 'Nb': 4, 'Nu': 2, 'Nd': 2 }, shift=-1., cross_section=.5)
    ]
}

def expand_features(steer:dict)->list[dict]:
    """The entries of features/interpret with a TTree, with one entry per name of the entries using `names`"""
    features = []

    for entry in steer['features']['interpret']:
        if 'tree' not in entry:
            continue

        for name in (entry['names'] if 'names' in entry else [entry['name']]):
            features.append({ **{ key: value for key, value in entry.items() if key != 'names' }, 'name': name })

    return features

def passing_distribution(cut:dict)->tuple[float, float]:
    """Mean and width of a normal distribution which mostly passes a cut. Integer columns are rounded (see
    feature_values), so that about 90 % of the events pass gte and eq, which only make sense for these"""
    match cut['operator']:
        case 'within_bounds':
            return (cut['lower'] + cut['upper']) / 2, (cut['upper'] - cut['lower']) / 8
        case 'lte':
            return .8 * cut['value'], .05 * cut['value']
        case 'gte':
            return cut['value'] + .5, .7
        case 'eq':
            return cut['value'], .3
        case operator:
            raise ValueError(f'Unknown operator {operator}')

def feature_values(feature:dict, cuts:dict[str, dict], shifts:np.ndarray, rng:np.random.Generator)->np.ndarray:
    size = len(shifts)

    if feature['name'] in cuts:
        mean, width = passing_distribution(cuts[feature['name']])
        values = rng.normal(mean, width, size)
    else:
        values = rng.normal(SHIFT_MEAN + shifts * SHIFT_WIDTH, SHIFT_WIDTH)

    # all values are positive, unsigned columns must not wrap around
    if feature.get('dtype', 'float32').startswith('float'):
        return values.astype('f4')

    return np.rint(np.clip(values, 0, None)).astype('i4')

def component_columns(components:list[Component], processes:dict[str, int], size:int, rng:np.random.Generator)->dict[str, np.ndarray]:
    """Physics process and final state of each event"""
    fractions = np.array([component.fraction for component in components])
    index = rng.choice(len(components), size=size, p=fractions / fractions.sum())

    def per_component(fn, dtype):
        return np.array([fn(component) for component in components], dtype=dtype)[index]

    columns = {
        'event': np.arange(size, dtype='i4'),
        'shift': per_component(lambda c: c.shift, 'f8'),
        'process': per_component(lambda c: processes[c.process], 'i4'),
        'cross_section': per_component(lambda c: c.cross_section, 'f4'),
        'n_b_from_higgs': per_component(lambda c: c.n_b_from_higgs, 'i4'),
        'polarization_code': rng.choice(POLARIZATION_CODES, size=size).astype('i4')
    }

    for i, name in enumerate(FSC_KEYS):
        columns[f'fsc{i}'] = per_component(lambda c: c.final_state.get(name, 0), 'i4')

    return columns

def write_tree(path:Path, tree:str, columns:dict[str, np.ndarray], definitions:dict[str, str], branches:list[str]):
    """Appends a TTree to path. RDataFrame gives the branches the layout of a tree written by Marlin"""
    import ROOT

    df = ROOT.RDF.FromNumpy(columns)
    for name, expression in definitions.items():
        df = df.Define(name, expression)

    options = ROOT.RDF.RSnapshotOptions()
    options.fMode = 'UPDATE' if path.exists() else 'RECREATE'

    df.Snapshot(tree, str(path), ROOT.std.vector('string')(branches), options)

def write_source(path:Path, name:str, steer:dict, processes:dict[str, int], cuts:dict[str, dict], rng:np.random.Generator):
    import ROOT

    # required to write std::map; ROOT writes the generated files to the working directory
    ROOT.gInterpreter.GenerateDictionary('map<int,int>', 'map')

    base = component_columns(SOURCES[name], processes, N_EVENTS_PER_SOURCE, rng)
    final_state_counts = ','.join(f'{{{key},fsc{i}}}' for i, key in enumerate(FSC_KEYS.values()))

    write_tree(path, 'FinalStates', { key: value for key, value in base.items() if key != 'shift' },
               { 'final_state_counts': f'std::map<int,int>{{{final_state_counts}}}' },
               ['event', 'process', 'cross_section', 'n_b_from_higgs', 'polarization_code', 'final_state_counts'])

    trees:dict[str, list[dict]] = {}
    for feature in expand_features(steer):
        if feature['tree'] != 'FinalStates':
            trees.setdefault(feature['tree'], []).append(feature)

    for tree, features in trees.items():
        columns, definitions, branches = {}, {}, []

        for feature in features:
            branch = feature.get('branch', feature['name'])

            if isinstance(feature.get('columns'), list):
                # four-vector with sub-branches fCoordinates.f{X,Y,Z,T}
                branch = branch.split('/')[0]
                for component in 'xyzt':
                    columns[f'{branch}_{component}'] = feature_values(feature, cuts, base['shift'], rng)

                definitions[branch] = f'ROOT::Math::PxPyPzEVector({branch}_x, {branch}_y, {branch}_z, {branch}_t)'
            else:
                columns[branch] = feature_values(feature, cuts, base['shift'], rng)

            branches.append(branch)

        write_tree(path, tree, columns, definitions, branches)

def steering_for_dummy_data(steer:dict, output_dir:Path)->dict:
    """A copy of the steering file reading the dummy data, which also runs on its own in an empty directory:
    the MVA data is written, and a single minimal training replaces the hyperparameter search"""
    steer = copy.deepcopy(steer)

    for key in ['event_categories', 'processes']:
        steer[key] = f'$REPO_ROOT/config/{steer[key]}'

    for source in steer['sources']:
        source['path'] = str(output_dir / 'items' / source['name'])
        source['root_files'] = str(output_dir / 'root' / source['name'] / '*.root')

    for action in steer['actions']:
        match action['type']:
            case 'WriteMVAData':
                action['disabled'] = False
            case 'SklearnMulticlassTraining':
                action['disabled'] = False
                action['hyperparams'] = {
                    'n_estimators': 10,
                    'max_depth': 3,
                    'learning_rate': .3
                }
            case 'SklearnMulticlassHyperparamTraining':
                action['disabled'] = True

    return steer

def expected_outputs(steer:dict)->list[str]:
    """All files written to the working directory by a run of the steering file"""
    hypothesis = steer['hypothesis']

    files = [f'{hypothesis}.{source["name"]}.h5' for source in steer['sources'] if not source.get('disabled', False)]

    for mva in steer['mvas']:
        files += [mva['data_file'], mva['mva_file']]

    for action in steer['actions']:
        if action.get('disabled', False):
            continue

        files += [action[key] for key in ['cache', 'cut_file', 'plot_file'] if key in action]

        if 'file' in action:
            if action['type'] == 'CreateCutflowTable':
                stem = Path(action['file']).stem
                files += [f'{stem}.pdf', f'{stem}_efficiency.pdf', f'{stem}_frac.pdf', f'{stem}_counts.csv']
            else:
                files.append(action['file'])

    return [file.replace('$hypothesis', hypothesis) for file in files]

def create(steering_file:Path, output_dir:Path)->Path:
    with open(steering_file, encoding='utf-8') as f:
        steer = yaml.safe_load(f)

    with open(steering_file.parent / steer['processes'], encoding='utf-8') as f:
        processes = json.load(f)

    cuts = { cut['quantity']: cut for cut in steer['cuts']['preselection'] if not cut.get('disabled', False) }
    rng = np.random.default_rng(SEED)

    for source in steer['sources']:
        name = source['name']
        directory = output_dir / 'root' / name
        directory.mkdir(parents=True)
        write_source(directory / f'{name}.root', name, steer, processes, cuts, rng)

    output_file = output_dir / steering_file.name
    with open(output_file, 'w', encoding='utf-8') as f:
        yaml.safe_dump(steering_for_dummy_data(steer, output_dir), f, allow_unicode=True, sort_keys=False)

    return output_file

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('steering_file', type=Path)
    parser.add_argument('output_dir', type=Path)
    args = parser.parse_args()

    os.makedirs(args.output_dir, exist_ok=True)
    print(create(args.steering_file.resolve(), args.output_dir.resolve()))
