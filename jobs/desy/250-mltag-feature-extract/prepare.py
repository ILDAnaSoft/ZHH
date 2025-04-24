# %%
from glob import glob
import sys, os, dotenv
import os.path as osp

this_dir = osp.dirname(__file__)
job_parameters = osp.join(this_dir, 'job_parameters')
dotenvfile = osp.join(osp.dirname(osp.dirname(job_parameters)), '.env')

dotenv.load_dotenv(dotenvfile)

TASK_NAME = sys.argv[1]
TASK_ROOT = os.environ['TASK_ROOT']

if __name__ == '__main__':
    os.makedirs(job_parameters, exist_ok=True)

    flavors = ['b', 's', 'c', 'u', 'd']
    
    def parse_line(f, fl_str, i):
        res = ''
        for __TASK_NAME, CPIDName in [
            (f'{TASK_NAME}/cpid', 'TMVA_BDT_MC_12bins_singleP'),
            (f'{TASK_NAME}/mcpid', 'MCPID')
        ]:
            res += f'{f},{fl_str}_{i},{dotenvfile},{CPIDName},{__TASK_NAME}\n'
        
        return res
    
    def parse_lines(files, fl_str):
        out = ''
        
        for i, f in enumerate(files):
            out += f'{parse_line(f, fl_str, i)}'
            
        return out

    for flavor in flavors:
        fl_str = (flavor * 4)
        files = glob(f'/pnfs/desy.de/ilc/prod/ilc/mc-2020/ild/dst-merged/250-SetA/flavortag/ILD_l5_o1_v02/v02-02/*_{fl_str}.*')
        files.sort()
        
        with open(f'{job_parameters}/paramlist_{flavor}.queue', 'w') as tfile:
            for i, f in enumerate(files):
                for __TASK_NAME in [f'{TASK_NAME}/cpid', f'{TASK_NAME}/mcpid']:
                    if not osp.isfile(f'{TASK_ROOT}/{__TASK_NAME}/output/FT_{fl_str}_{i}.slcio'):
                        tfile.write(f'{parse_line(f, fl_str, i)}')
            
        with open(f'{job_parameters}/paramlist_{flavor}_test.queue', 'w') as tfile:
            tfile.write(parse_lines([files[0]], fl_str))
            
    with open(f'{job_parameters}/paramlist_ud.queue', 'w') as tfile:
        with open(f'{job_parameters}/paramlist_u.queue', 'r') as readfile:
            tfile.write(readfile.read())
        
        with open(f'{job_parameters}/paramlist_d.queue', 'r') as readfile:
            tfile.write(readfile.read())

    with open(f'{job_parameters}/paramlist_ud_test.queue', 'w') as tfile:
        with open(f'{job_parameters}/paramlist_u_test.queue', 'r') as readfile:
            tfile.write(readfile.read())
        
        with open(f'{job_parameters}/paramlist_d_test.queue', 'r') as readfile:
            tfile.write(readfile.read())

    for (suffix_target, suffix_source) in (
        ('all', ''),
        ('test', '_test')
    ):
        with open(f'{suffix_target}.queue', 'w') as tfile:
            for type in ['b', 'c', 's', 'ud']:
                with open(f'{job_parameters}/paramlist_{type}{suffix_source}.queue', 'r') as readfile:
                    tfile.write(readfile.read())

    os.remove(f'{job_parameters}/paramlist_u.queue')
    os.remove(f'{job_parameters}/paramlist_d.queue')
    os.remove(f'{job_parameters}/paramlist_u_test.queue')
    os.remove(f'{job_parameters}/paramlist_d_test.queue')

    # Write job files
    for log_dir in [
        f'{TASK_ROOT}/{TASK_NAME}/cpid/logs',
        f'{TASK_ROOT}/{TASK_NAME}/mcpid/logs'
    ]:
        os.makedirs(log_dir, exist_ok=True)

    for target_suffix in ['all', 'test']:
        with open(f'{this_dir}/{target_suffix}.sub', 'w') as submit_file:
            with open(f'{this_dir}/job_template', 'r') as tfile:
                template = tfile.read()
            
            content = template.replace('$TASK_ROOT', TASK_ROOT)
            content = content.replace('$TARGET_SUFFIX', target_suffix)
            
            submit_file.write(content)

