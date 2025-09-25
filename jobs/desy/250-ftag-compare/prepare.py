# %%
from glob import glob
import sys, os
import os.path as osp

files = glob('/data/dust/user/bliewert/zhh/FastSimSGV/250-ftag-fast-perf/*.slcio')
files.sort()

OUTPUT_BDIR = '/data/dust/user/bliewert/FTag3'
REPO_ROOT = '/afs/desy.de/group/flc/pool/bliewert/MarlinWorkdirs/ZHH'

with open(f'queue.txt', 'w') as submit_file:
    for file in files:
        submit_file.write(f'{OUTPUT_BDIR},{osp.basename(file)},{REPO_ROOT},{file}\n')

