from workflows.analysis.utils.WhizardTaskReport import inspect_whizard_outputs
from tabulate import tabulate
import os.path as osp
import argparse

if __name__=="__main__":
    parser = argparse.ArgumentParser(description='Tool to check the output of a WhizardEventGeneration task. Gives cross section, number of simulated generated events per process and polarization combination. Example usage: python inspect_whizard_output.py --results=/data/dust/user/$(whoami)/zhh/WhizardEventGeneration/550-llbb-fast-perf')
    parser.add_argument('results', type=str, help='path to the ROOT of a WhizardEventGeneration task')
    args = parser.parse_args()
    
    root = str(args.results)
    
    if not osp.isdir(root):
        raise Exception(f'{root} is not a valid directory')
    
    reports = inspect_whizard_outputs(root)
    if not len(reports):
        raise Exception(f'No results')
    
    reports.sort(key=lambda rep: rep['process'])
    
    table = []
    unit = ''
    for rep in reports:
        pol1 = rep['beamPol1']
        pol2 = rep['beamPol2']
        
        assert(pol1 in [-1, +1] and pol2 in [-1, +1])
        assert(rep['beam1'] == 'e-' and rep['beam2'] == 'e+')
        
        pol_string = f"e{'L' if pol1 < 0 else 'R'}.p{'L' if pol2 < 0 else 'R'}"
        cunit = rep['cross_section_unit']
        
        if unit == '':
            unit = cunit
        else:
            assert(unit == cunit)
        
        table.append([
            rep['sqrt_s'],
            rep['process'],
            pol_string,
            rep['n_gen'],
            f"{rep['cross_section']:.2e} +- {rep['cross_section_error']:.1e}"
        ])
    
    print(f"Generated using whizard {rep['whizard_version']}\n")
    print(tabulate(table, headers=[
        'sqrt_s',
        'Process',
        'Beam configuration',
        'Ngenerated',
        f'Cross section [{unit}]'
    ]))
    
    
    