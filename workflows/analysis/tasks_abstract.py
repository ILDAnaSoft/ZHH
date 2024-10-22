from analysis.framework import HTCondorWorkflow
import luigi, law, json, os, uuid
import os.path as osp
from law.util import flatten
from law import LocalFileTarget
from phc import ShellTask, BaseTask
from typing import Optional, Union

class MarlinJob(ShellTask):
    """Abstract class for Marlin jobs
    
    The parameters for running Marlin can be set here
    or overwritten in a child class by a custom imple-
    mentation of get_steering_parameters.
    """
    
    executable = 'Marlin'
    
    n_events_max:Optional[int] = None
    n_events_skip:Optional[int] = None
    
    steering_file:str = '$REPO_ROOT/scripts/ZHH_v2.xml'
    
    check_output_root_ttrees:Optional[list[tuple[str,str]]] = None
    # Optional: list of tuples of structure (file-name.root, TTree-name)
    
    check_output_files_exist:Optional[list[str]] = None
    
    def get_steering_parameters(self, branch:int, branch_value:Union[tuple[str,int,int], str]) -> dict:
        """The branch map self.branch_map is a dictionary
        branch => value where value has one of the following form:
        
        a) tuples: (input_file:str, n_events_skip:int, n_events_max:int)
        b) string: input_file

        Args:
            branch (int): _description_
            branch_value (Union[tuple[str,int,int], str]): _description_

        Returns:
            dict: _description_
        """
        if isinstance(branch_value, tuple):
            input_file, n_events_skip, n_events_max = branch_value
        else:
            input_file, n_events_skip, n_events_max = branch_value, self.n_events_skip, self.n_events_max
        
        steering = {
            'executable': self.executable,
            'steering_file': self.steering_file,
            'input_file': input_file,
            'n_events_skip': n_events_skip,
            'n_events_max': n_events_max,            
        }
        
        return steering
    
    def get_target_and_temp(self, branch):
        return (
            f'{self.htcondor_output_directory().path}/{branch}',
            f'{self.htcondor_output_directory().path}/{branch}-{str(uuid.uuid4())}'
        )
    
    def build_command(self, fallback_level):
        branch = self.branch
        steering = self.get_steering_parameters(branch, self.branch_map[branch])
        
        executable, steering_file, input_file, n_events_skip, n_events_max = steering.values()
        
        target, temp = self.get_target_and_temp(branch)
        os.makedirs(osp.dirname(target), exist_ok=True)
        
        # Check if sample belongs to new or old MC production to change MCParticleCollectionName
        mcp_col_name = 'MCParticlesSkimmed' if '/hh/' in input_file else 'MCParticle'
        
        cmd =  f'source $REPO_ROOT/setup.sh'
        cmd += f' && echo "Starting Marlin at $(date)"'
        cmd += f' && mkdir -p "{temp}" && cd "{temp}"'

        # Marlin fix for SkipNEvents=0
        if n_events_skip == 0:
            n_events_max = n_events_max + 1
        
        cmd += f' && ( {executable} {steering_file} --constant.ILDConfigDir="$ILD_CONFIG_DIR" --global.MaxRecordNumber={str(n_events_max)} --global.LCIOInputFiles={input_file} --global.SkipNEvents={str(n_events_skip)} --constant.OutputDirectory=. --constant.MCParticleCollectionName={mcp_col_name} || true )'
        cmd += f' && echo "Finished Marlin at $(date)"'
        cmd += f' && sleep 2'
        
        if self.check_output_root_ttrees is not None:
            for name, ttree in self.check_output_root_ttrees:
                cmd += f' && is_root_readable ./{name} {ttree}'
                
        if self.check_output_files_exist is not None:
            for name in self.check_output_files_exist:
                cmd += f' && [[ -f ./{name} ]]'
        
        cmd += f' && echo "{self.branch_map[self.branch]}" >> Source.txt'
        cmd += f' && cd .. && mv "{temp}" "{target}"'

        return cmd