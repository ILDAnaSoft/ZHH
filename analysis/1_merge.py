from glob import glob
from configurations import llhh1_lvbbqq

prefix1 = '/home/ilc/bliewert/jobresults/550-llhh-ana'
prefix2 = '/home/ilc/bliewert/jobresults/550-2l4q-ana'

files = glob(f'{prefix1}/*.root') + glob(f'{prefix2}/*.root')
output = list(filter(lambda path: any([elem in path for elem in ['e1e1hh', 'e2e2hh', 'P6f']]), files))
output.sort()

print(f'Found {len(output)} output files')

from configurations import llhh1_lvbbqq
WORK_ROOT = '/home/ilc/bliewert/jobresults/analysis'

llhh1_lvbbqq.initialize(WORK_ROOT, root_files=output, trees=['FinalStates', 'EventObservablesLL', 'KinFitLLZHH'])

rf = llhh1_lvbbqq.rf