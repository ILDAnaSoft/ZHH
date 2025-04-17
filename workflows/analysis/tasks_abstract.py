from analysis.framework import HTCondorWorkflow
from analysis.utils import SGVSteeringModifier
import law, os, uuid
import os.path as osp
from typing import Optional, Union, cast
from .utils.types import SGVOptions
from zhh import ProcessIndex
from .utils import ShellTask, BaseTask, RealTimeLoggedTask

class MarlinJob(ShellTask, HTCondorWorkflow, law.LocalWorkflow):
    """Abstract class for Marlin jobs
    
    The parameters for running Marlin can be set here
    or overwritten in a child class by a custom imple-
    mentation of get_steering_parameters.
    """
    
    executable = 'Marlin'
    
    n_events_max:Optional[int] = None
    n_events_skip:Optional[int] = None
    
    # Append to these of you want to set additional constants or globals
    # You may define a pre_run_command to do so in a dynamic way 
    constants:list[tuple[str,str]] = []
    globals:list[tuple[str,str]] = []
    
    steering_file:str = '$REPO_ROOT/scripts/prod.xml'
    
    # Optional: list of tuples of structure (file-name.root, TTree-name)
    check_output_root_ttrees:Optional[list[tuple[str,str]]] = None 
    
    # Optional: list of files
    check_output_files_exist:Optional[list[str]] = None
    
    def get_steering_parameters(self) -> dict:
        """The branch map self.branch_map is a dictionary
        branch => value where value has one of the following form:
        
        a) tuples: (input_file:str, n_events_skip:int, n_events_max:int)
        b) string: input_file

        Returns:
            dict: _description_
        """
        
        branch_value = self.branch_map[self.branch]
        
        if isinstance(branch_value, tuple):
            input_file, n_events_skip, n_events_max, mcp_col_name = branch_value
        elif isinstance(branch_value, dict):
            n_events_skip, n_events_max = self.n_events_skip, self.n_events_max
            
            input_file = branch_value['location']
            mcp_col_name = branch_value['mcp_col_name']
        else:
            raise Exception('Invalid format of branch value')
        
        steering = {
            'executable': self.executable,
            'steering_file': self.steering_file,
            'input_file': input_file,
            'n_events_skip': n_events_skip,
            'n_events_max': n_events_max,
            'mcp_col_name': mcp_col_name
        }
        
        return steering
    
    def get_target_and_temp(self):
        return (
            f'{self.htcondor_output_directory().path}/{self.branch}',
            f'{self.htcondor_output_directory().path}/{self.branch}-{str(uuid.uuid4())}'
        )
    
    def parse_marlin_globals(self) -> str:
        globals = filter(lambda tup: tup[0] not in ['MaxRecordNumber', 'LCIOInputFiles', 'SkipNEvents'], self.globals)
        return ' '.join([f'--global.{key}="{value}"' for key, value in globals])
    
    def parse_marlin_constants(self) -> str:
        return ' '.join([f'--constant.{key}="{value}"' for key, value in self.constants])
    
    def build_command(self, fallback_level):
        steering = self.get_steering_parameters()
        
        executable = steering['executable']
        steering_file = steering['steering_file']
        input_file = steering['input_file']
        n_events_skip = steering['n_events_skip']
        n_events_max = steering['n_events_max']
        
        target, temp = self.get_target_and_temp()
        os.makedirs(osp.dirname(target), exist_ok=True)
        
        cmd =  f'source $REPO_ROOT/setup.sh'
        cmd += f' && echo "Starting Marlin at $(date)"'
        cmd += f' && mkdir -p "{temp}" && cd "{temp}"'
            
        str_max_record_number = f' --global.MaxRecordNumber={str(n_events_max)}' if (n_events_max is not None and n_events_max != 0)  else ''
        str_skip_n_events = f' --global.SkipNEvents={str(n_events_skip)}' if (n_events_skip is not None and n_events_skip != 0) else ''
        
        cmd += f' && ( {executable} {steering_file} {self.parse_marlin_constants()} {self.parse_marlin_globals()}{str_max_record_number}{str_skip_n_events} --global.LCIOInputFiles={input_file} || true )'
        cmd += f' && echo "{input_file}" >> Source.txt'
        cmd += f' && echo "Finished Marlin at $(date)"'
        cmd += f' && ( sleep 2'
        
        if self.check_output_root_ttrees is not None:
            for name, ttree in self.check_output_root_ttrees:
                cmd += f' && ( is_root_readable ./{name} {ttree} && echo "Success: TTree <{ttree}> in file <{name}> exists" ) '
                
        if self.check_output_files_exist is not None:
            for name in self.check_output_files_exist:
                cmd += f' && [[ -f ./{name} ]] && echo "Success: File <{name}> exists"'
        
        cmd += f' && cd .. && mv "{temp}" "{target}" )'

        return cmd
    
    def output(self):
        return self.local_directory_target(self.branch)

class FastSimSGVExternalReadJob(ShellTask, HTCondorWorkflow, law.LocalWorkflow):
    """Abstract class for fast simulation jobs using SGV, reading in
    LCIO/STDHEP and out-putting LCIO files
    
    We assume a working installation of SGV using the -EXTREAD option
    with LCIO/STDHEP support. This can be achieved by following the
    steps in https://gitlab.desy.de/mikael.berggren/sgv specifically
    in the samples directory.
    
    We assume sourcing sgv_env and calling the executable usesgvlcio
    works provided the input_file exists. We thus copy the directory
    including the executable to the working node, symlink the input
    file to a location given by input_file and expect the output at
    output_file (again within the current working directory).
    
    Assign the location to SGV_DIR in the workflows/analysis/.env file
    
    The parameters for running SGV can be set here or overwritten in a
    child class by a custom implementation of get_steering_file.
    """
    
    executable = '$SGV_DIR/tests/usesgvlcio.exe'
    
    # this can be changed, if desired
    steering_file_src = '$SGV_DIR/tests/sgv.steer'
    
    # this must fit the compilation of usesgvlcio (here: default)
    # copied from steering_file_src to the working directory
    steering_file_name = 'fort.17'
    
    sgv_env = '$SGV_DIR/sgvenv.sh'
    sgv_input = 'input.slcio' # this must fit the steering file, also the GENERATOR_INPUT_TYPE
    sgv_output = 'sgvout.slcio' # this must fit the steering file
    
    # False to allow for checks
    tmp_steering_name = 'sgv-final.steer'
    tmp_dir: Optional[str] = None
    
    def get_steering_file(self)->str:
        """Default implementation for creating a SGV steering
        file. Reads in steering_file_src, merges any options in
        input_options and returns the content for the steering
        file.

        Args:
            branch (int): _description_
            input_file (str): _description_

        Returns:
            str: merged steering file content
        """
        
        input_file, input_options = cast(tuple[str, SGVOptions], self.branch_data)
        
        # change the name of the expected input file to SGV if it was supplied
        # in input_options
        if isinstance(input_options, dict) and 'external_read_generation_steering.INPUT_FILENAMES' in input_options:
            self.sgv_input = input_options['external_read_generation_steering.INPUT_FILENAMES']
        
        modifier = SGVSteeringModifier(osp.expandvars(self.steering_file_src))
        
        return modifier.merge_properties(input_options if isinstance(input_options, dict) else {})
    
    def get_temp_dir(self):
        if not self.tmp_dir:
            output_path = cast(str, self.output().path)
            self.tmp_dir = f'{osp.dirname(output_path)}/TMP-{osp.splitext(osp.basename(output_path))[0]}-{str(uuid.uuid4())}'
            
        return self.tmp_dir
    
    def build_command(self, **kwargs):    
        input_file, input_options = cast(tuple[str, SGVOptions], self.branch_data)
        target_path = str(self.output().path)
        
        steering_file_content = self.get_steering_file()
        with open(f'{kwargs["cwd"]}/{self.tmp_steering_name}', 'w') as sf:
            sf.write(steering_file_content)
        
        # create steering file: parse source file and merge input_options into it
        
        cmd  = f'source $REPO_ROOT/setup.sh && source "{self.sgv_env}"'
        cmd += f' && echo "SRC={input_file} DST={target_path}"'
        cmd += f' && cp -R $(dirname {self.executable})/* .'
        cmd += f' && ( [[ -f {self.steering_file_name} ]] && rm {self.steering_file_name} && echo "Existing steering file removed" || echo "No existing steering file removed" )'
        cmd += f' && mv "{self.tmp_steering_name}" "{self.steering_file_name}"'
        cmd += f' && ln -s "{input_file}" {self.sgv_input}'
        cmd += f' && echo "Starting SGV at $(date)"'
        cmd += f' && ( ./{osp.basename(self.executable)}'
        cmd += f' && echo "Finished SGV at $(date)"'
        cmd += f' && echo "Moving from worker node to destination"'
        cmd += f' && mv "{self.sgv_output}" "{target_path}"'
        cmd += f' )'
        #cmd += f' && rm -rf "" ) '
        
        return cmd
    
    def output(self):
        return self.local_target(f'{self.branch}.slcio')
    
    def run(self, **kwargs):
        ShellTask.run(self, cwd=self.get_temp_dir(), **kwargs)