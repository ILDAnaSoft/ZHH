from analysis.framework import HTCondorWorkflow
import law, os, uuid
import os.path as osp
from zhh import ShellTask
from typing import Optional, Union, List

class MarlinJob(ShellTask, HTCondorWorkflow, law.LocalWorkflow):
    """Abstract class for Marlin jobs
    
    The parameters for running Marlin can be set here
    or overwritten in a child class by a custom imple-
    mentation of get_steering_parameters.
    """
    
    executable = 'Marlin'
    
    n_events_max:Optional[int] = None
    n_events_skip:Optional[int] = None
    
    # Use add_constant and add_global to append to these
    # to avoid conflicts
    constants:List[tuple[str,str]] = []
    globals:List[tuple[str,str]] = []
    
    steering_file:str = '$REPO_ROOT/scripts/prod.xml'
    
    # Optional: list of tuples of structure (file-name.root, TTree-name)
    check_output_root_ttrees:Optional[list[tuple[str,str]]] = None 
    
    # Optional: list of files
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
    
    def parse_marlin_globals(self) -> str:
        globals = filter(lambda tup: tup[0] not in ['MaxRecordNumber', 'LCIOInputFiles', 'SkipNEvents'], self.globals)
        return ' '.join([f'--global.{key}="{value}"' for key, value in globals])
    
    def parse_marlin_constants(self) -> str:
        return ' '.join([f'--constant.{key}="{value}"' for key, value in self.constants])
    
    def build_command(self, fallback_level):
        branch = self.branch
        steering = self.get_steering_parameters(branch, self.branch_map[branch])
        
        executable, steering_file, input_file, n_events_skip, n_events_max = steering.values()
        
        target, temp = self.get_target_and_temp(branch)
        os.makedirs(osp.dirname(target), exist_ok=True)
        
        # Check if sample belongs to new or old MC production to change MCParticleCollectionName
        # TODO: Check if this is sufficient
        mcp_col_name = 'MCParticlesSkimmed' if '/mc-2020/' in input_file else 'MCParticle'
        
        cmd =  f'source $REPO_ROOT/setup.sh'
        cmd += f' && echo "Starting Marlin at $(date)"'
        cmd += f' && mkdir -p "{temp}" && cd "{temp}"'

        # Marlin fix for SkipNEvents=0
        # If SkipNEvents=0 is supplied, Marlin will actually process the event header as event 0
        # and increse the event counter by one. To keep the counting correct, we use this workaround.
        if n_events_skip == 0:
            n_events_max = n_events_max + 1
            
        str_max_record_number = f' --global.MaxRecordNumber={str(n_events_max)}' if n_events_max is not None else ''
        str_skip_n_events = f' --global.SkipNEvents={str(n_events_skip)}' if n_events_skip is not None else ''
        
        cmd += f' && ( {executable} {steering_file} {self.parse_marlin_constants()} {self.parse_marlin_globals()}{str_max_record_number}{str_skip_n_events} --constant.MCParticleCollectionName={mcp_col_name} || true )'
        cmd += f' && echo "Finished Marlin at $(date)"'
        cmd += f' && {{ sleep 2'
        
        if self.check_output_root_ttrees is not None:
            for name, ttree in self.check_output_root_ttrees:
                cmd += f' && is_root_readable ./{name} {ttree}'
                
        if self.check_output_files_exist is not None:
            for name in self.check_output_files_exist:
                cmd += f' && [[ -f ./{name} ]]'
        
        cmd += f' && echo "{self.branch_map[self.branch]}" >> Source.txt'
        cmd += f' && cd .. && mv "{temp}" "{target}" }} || rm -rf "{temp}"'

        return cmd