# %%
from glob import glob
import sys, os
import os.path as osp
from dotenv import dotenv_values

TASK_NAME = 'FeatureExtraction'

this_dir = os.path.abspath('')
job_parameters = osp.join(this_dir, 'job_parameters')
os.makedirs(job_parameters, exist_ok=True)

# %%
flavors = ['b', 's', 'c', 'u', 'd']
dotenvfile = osp.join(osp.dirname(osp.dirname(job_parameters)), '.env')

def parse_lines(files, fl_str):
    out = ''
    
    for i, f in enumerate(files):
        out += f'{f},{fl_str}_{i},{dotenvfile}\n'
        
    return out

for flavor in flavors:
    fl_str = (flavor * 6)
    files = glob(f'/pnfs/desy.de/ilc/prod/ilc/mc-opt-3/ild/dst-merged/500-TDR_ws/flavortag/ILD_l5_o1_v02/v02-00-01/*.P{fl_str}.*')
    files.sort()
    
    with open(f'{job_parameters}/paramlist_{flavor}.txt', 'w') as tfile:
        tfile.write(parse_lines(files, fl_str))
        
    with open(f'{job_parameters}/paramlist_{flavor}_test.txt', 'w') as tfile:
        tfile.write(parse_lines([files[0]], fl_str))
        
with open(f'{job_parameters}/paramlist_ud.txt', 'w') as tfile:
    with open(f'{job_parameters}/paramlist_u.txt', 'r') as readfile:
        tfile.write(readfile.read())
    
    with open(f'{job_parameters}/paramlist_d.txt', 'r') as readfile:
        tfile.write(readfile.read())

with open(f'{job_parameters}/paramlist_ud_test.txt', 'w') as tfile:
    with open(f'{job_parameters}/paramlist_u_test.txt', 'r') as readfile:
        tfile.write(readfile.read())
    
    with open(f'{job_parameters}/paramlist_d_test.txt', 'r') as readfile:
        tfile.write(readfile.read())

for (suffix_target, suffix_source) in (
    ('all', ''),
    ('test', '_test')
):
    with open(f'{job_parameters}/paramlist_{suffix_target}.txt', 'w') as tfile:
        for type in ['b', 'c', 's', 'ud']:
            with open(f'{job_parameters}/paramlist_{type}{suffix_source}.txt', 'r') as readfile:
                tfile.write(readfile.read())

os.remove(f'{job_parameters}/paramlist_u.txt')
os.remove(f'{job_parameters}/paramlist_d.txt')
os.remove(f'{job_parameters}/paramlist_u_test.txt')
os.remove(f'{job_parameters}/paramlist_d_test.txt')

# %% Write job files

config = {
    **dotenv_values(dotenvfile),  # load shared development variables
    **os.environ,  # override loaded values with environment variables
}

os.makedirs(f'{config["TASK_ROOT"]}/{TASK_NAME}/logs', exist_ok=True)

for target_suffix in ['all', 'test']:
    with open(f'{this_dir}/submit_{target_suffix}.sub', 'w') as submit_file:
        with open(f'{this_dir}/job_template', 'r') as tfile:
            template = tfile.read()
        
        content = template.replace('$LOG_ROOT', f'{config["TASK_ROOT"]}/{TASK_NAME}/logs')
        content = content.replace('$TARGET_SUFFIX', target_suffix)
        
        submit_file.write(content)
# %%
