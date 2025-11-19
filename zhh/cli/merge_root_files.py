import argparse
from glob import glob
from zhh import DataSource

if __name__=="__main__":
    parser = argparse.ArgumentParser()
    
    parser.add_argument('work_root',  type=str, help='directory where the output files will be stored')
    parser.add_argument('ttrees',  type=str, help='comma-separated list of TTree names to merge')
    parser.add_argument('--dirs',  type=str, required=False, help='comma-separated list of directories containing root files')
    parser.add_argument('--files',  type=str, required=False, help='comma-separated list of root files')
    parser.add_argument('--recursive',  type=bool, default=False, required=False, help='comma-separated list of root files')
    
    args = parser.parse_args()
    
    work_root = args.work_root
    ttrees = args.ttrees.split(',')
    root_dirs = args.dirs
    root_files = args.files
    recursive = args.recursive
    
    assert(root_dirs is not None or root_files is not None)
    
    if root_files is not None:
        files = list(map(lambda x: x.strip(), root_files.split(',')))
    else:
        files = []
        for root_dir in root_dirs.split(','):
            files += glob(f'{root_dir}/*.root', recursive=recursive)
    
    #files = list(filter(lambda path: any([elem in path for elem in ['e1e1hh', 'e2e2hh', 'P6f']]), files))
    files.sort()

    print(f'Merging {len(files)} output files')
    
    assert(len(files))
    
    analysis = DataSource(work_root)
    analysis.combine(trees=ttrees, root_files=files)