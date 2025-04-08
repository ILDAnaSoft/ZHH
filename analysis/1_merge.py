from common import *

if __name__=="__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--name', type=str, required=True, help='name of the analysis channel')
    parser.add_argument('--cuts', type=str, required=True, help='name of the preselection cuts; llhh, vvhh, qqhh')
    parser.add_argument('--root_dirs',  type=str, required=True, help='comma-separated list of directories containing root files')
    parser.add_argument('--work_root',  type=str, required=True, help='directory where the output files will be stored')
    
    args = parser.parse_args()
    
    name = str(args.name)
    cuts = str(args.cuts)
    root_dirs = str(args.root_dirs)
    work_root = str(args.work_root)
    
    files = []
    for root_dir in root_dirs.split(','):
        files += glob(f'{root_dir}/*.root')
    
    #files = list(filter(lambda path: any([elem in path for elem in ['e1e1hh', 'e2e2hh', 'P6f']]), files))
    files.sort()

    print(f'Found {len(files)} output files')
    
    analysis = AnalysisChannel(name, zhh_cuts(cuts))
    analysis.initialize(work_root, trees=['FinalStates', 'EventObservablesLL', 'KinFitLLNMC', 'KinFitLLZHH', 'KinFitLLZZH', 'KinFitLLZZZ'], root_files=files)