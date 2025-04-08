# example usage: python find_source_files.py --statistics=0.3

from glob import glob
import argparse

def get_process(path:str):
    return path.split('E550')[1].split('.P')[1].split('.')[0]

def get_proc_pol(path:str):
    return path.replace('Gwhizard-2_8_5.', '').split('E550')[1].split('.P')[1].split('.I')[0]

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument('--statistics', type=float, default=.3, help='fraction between 0 and 1 of the files to be used. at least one file per process and polarization will always be used')
    parser.add_argument('--source_masks', type=str, default='/home/ilc/bliewert/jobresults/550-hh-sgv/*.slcio', help='comma-separated list of globbable file masks for all source SLCIO files')

    args = parser.parse_args()

    statistics = float(args.statistics)
    source_masks = str(args.source_mask)
    files = []
    
    for source_mask in source_masks.split(','):
        files += glob(source_mask, recursive=True)
        #files = list(filter(lambda path: any([elem in get_process(path) for elem in ['e1e1', 'e2e2', 'v', 'l']]), files))
        
    files.sort()

    counts_max = {}
    counts_is = {}
    for file in files:
        proc_pol = get_proc_pol(file)
        if proc_pol not in counts_max:
            counts_is[proc_pol] = 0
            counts_max[proc_pol] = 0
        else:
            counts_max[proc_pol] += 1 

    output = []
    for file in files:
        proc_pol = get_proc_pol(file)
        if counts_is[proc_pol] == 0 or counts_is[proc_pol] < statistics * counts_max[proc_pol]:
            counts_is[proc_pol] += 1
            output.append(file)

    with open('files.txt', 'w') as f:
        f.write('\n'.join(output))
    
    print('Used files: Proc-pol: Used (Available)')
    for key in counts_is:
        print(f'{key}: {counts_is[key]} ({counts_max[key]})')