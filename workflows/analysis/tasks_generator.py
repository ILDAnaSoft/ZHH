from typing import Optional, cast, Union
from collections.abc import Callable
from .utils import ShellTask, BaseTask, RealTimeLoggedTask
from analysis.framework import HTCondorWorkflow, zhh_configs
from law import LocalFileTarget, LocalDirectoryTarget
from glob import glob
import os.path as osp
import subprocess, law, luigi

class WhizardEventGeneration(ShellTask, HTCondorWorkflow, law.LocalWorkflow):    
    def create_branch_map(self)->dict[int, dict]:        
        config = zhh_configs.get(str(self.tag))        
        assert(config.whizard_options is not None and len(config.whizard_options))
        
        whiz_ver = subprocess.check_output(['source $REPO_ROOT/setup.sh 2>&1 >/dev/null && whizard --version'], shell=True).split()[1].decode('utf-8')
        whizard_options = config.whizard_options
        
        branch_map = {}
        branch = 0        
        
        for whizard_option in whizard_options:            
            for beamPol1, beamPol2, pol_key in [
                ('-1', '-1', 'eL.pL'),
                ('-1',  '1', 'eL.pR'),
                ( '1', '-1', 'eR.pL'),
                ( '1',  '1', 'eR.pR')
            ]:
                properties = {
                    'WHIZARD_VERSION': whiz_ver,
                    'POLARIZATION_KEY': pol_key,
                    'BEAMPOL1': beamPol1,
                    'BEAMPOL2': beamPol2,
                    'PROCESS_NAME': whizard_option['process_name'],
                    'TEMPLATE_DIR': whizard_option['template_dir'],
                    'SINDARIN_FILE': whizard_option['sindarin_file']
                }
            
                branch_map[branch] = properties
                branch += 1
        
        return branch_map
    
    def outputBasename(self)->str:
        branch_data = cast(dict[str, str], self.branch_data)
        
        PROCESS_NAME = branch_data['PROCESS_NAME']
        WHIZARD_VERSION = branch_data['WHIZARD_VERSION']
        POLARIZATION_KEY = branch_data['POLARIZATION_KEY']
        
        return f'E550_TDR.P{PROCESS_NAME}.Gwhizard-{WHIZARD_VERSION}.{POLARIZATION_KEY}'
    
    def output(self):
        return [
            self.local_target(f'{self.outputBasename()}.slcio'),
            self.local_directory_target(self.outputBasename()),
        ]
    
    def build_command(self, **kwargs):
        # prepare inputs
        output_file, output_dir = self.output()
        BaseTask.touch_parent(output_file)
        output_dir.touch()
        
        branch_data = cast(dict[str, str], self.branch_data)
        
        TEMPLATE_DIR = osp.expandvars(branch_data['TEMPLATE_DIR'])
        SINDARIN_FILE = branch_data['SINDARIN_FILE']
        
        # prepare sindarin file
        sindarin = ''
        with open(osp.join(TEMPLATE_DIR, SINDARIN_FILE), 'r') as f:
            sindarin += f.read()
        
        sindarin = sindarin.replace('$OUTPUT_NAME', self.outputBasename())
        for prop in branch_data:
            sindarin = sindarin.replace(f'${prop}', branch_data[prop])
        
        with open(osp.join(self.output()[1].path, 'process.sin'), 'w') as f:
            f.write(sindarin)
        
        cmd =f"""source $REPO_ROOT/setup.sh
rm -f "{output_file.path}"
rm -f *.mod *.log *.smod *.lo *.f90 default* *.o
cp -R "{TEMPLATE_DIR}/." .
echo "Starting Whizard at $(date)"
whizard ./process.sin
echo "Finished Whizard at $(date)"
mv "{self.outputBasename()}.slcio" "{output_file.path}" """.replace('\n', ' && ')

        print('Executing command', cmd)
        
        return cmd
    
    def run(self, **kwargs):
        ShellTask.run(self, cwd=self.output()[1].path, keep_cwd=True, **kwargs)