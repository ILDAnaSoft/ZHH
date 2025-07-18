from analysis.framework import HTCondorWorkflow
from analysis.utils import SGVSteeringModifier
import law, luigi, uuid, numpy as np
import os.path as osp
from typing import Optional, cast
from .utils.types import SGVOptions
from zhh import ProcessIndex
from .utils import ShellTask, BaseTask

MarlinBranchValue = tuple[list[str]|str, int, int, int|None, int, str, str|None]
# [0]: input file: str if [6] is None, else must be list[str] input files
# [1]: chunk index of the given input file
# [2]: total number of chunks for the file
# [3]: n_events_skip
# [4]: n_events_max
# [5]: mcp_col_name
# [6]: str output basename or None

class AbstractMarlin(ShellTask, HTCondorWorkflow, law.LocalWorkflow):
    """Abstract class for Marlin jobs
    
    The parameters for running Marlin can be set here
    or overwritten in a child class by a custom imple-
    mentation of get_steering_parameters.
    
    Considers two targets per task branch:
    - output[0]: a directory, identified as the temp.
        working directory in which Marlin is run
    - output[1]: a file, identified by self.output_file
    """
    
    executable = 'Marlin'
    
    n_events_max:Optional[int] = None
    n_events_skip:Optional[int] = None
    
    # Append to these of you want to set additional constants or globals
    # You may define a pre_run_command to do so in a dynamic way 
    constants:list[tuple[str,str]] = []
    globals:list[tuple[str,str]] = []
    
    steering_file:str = '$REPO_ROOT/scripts/prod.xml'
    
    # file to be moved to the file output location (output[1])
    output_file:str = 'zhh_AIDA.root'
    
    # Optional: list of tuples of structure (file-name.root, TTree-name)
    check_output_root_ttrees:list[tuple[str,str]]|None = None 
    
    # Optional: list of files
    check_output_files_exist:list[str]|None = None
    
    # Optional: list of SLCIO files for which lcio_event_counter
    # must return successfully a number > 0
    check_output_lcio_files:list[str]|None = None
    
    def get_steering_parameters(self)->dict:
        """The branch map self.branch_map is a dictionary
        branch => value where value has one of the following form:
        
        a) tuples: (input_files:list[str], n_events_skip:int, n_events_max:int)
        b) string: input_file

        Returns:
            dict: _description_
        """
        
        branch_value = cast(MarlinBranchValue, self.branch_map[self.branch])
        assert isinstance(branch_value, tuple)
        
        input_files, n_chunk_of_sample, n_chunks_in_sample, n_events_skip, n_events_max, mcp_col_name, output_bname = branch_value
        
        n_events_skip = n_events_skip if (n_events_skip is None or n_events_skip > -1) else self.n_events_skip
        n_events_max  = n_events_max  if n_events_max  > -1 else self.n_events_max
        
        if isinstance(input_files, str):
            input_files = [input_files]
        
        steering = {
            'executable': self.executable,
            'steering_file': self.steering_file,
            'input_files': input_files,
            'n_events_skip': n_events_skip,
            'n_events_max': n_events_max,
            'mcp_col_name': mcp_col_name,
            'output_bname': output_bname
        }
        
        return steering
    
    def get_temp_dir(self):
        return f'{self.htcondor_output_directory().path}/TMP-{self.output_name()}'
    
    def output_name(self):
        branch_data = cast(MarlinBranchValue, self.branch_data)
        input_file = branch_data[0]
        output_bname = branch_data[-1]
        
        if isinstance(input_file, str):
            sample_filename = osp.basename(input_file)
            n_chunk = branch_data[1]
            n_chunks_in_sample = branch_data[2]
            
            return f'{osp.splitext(sample_filename)[0]}.{n_chunk}-{n_chunks_in_sample}-{str(self.branch)}'
        elif isinstance(input_file, list) and isinstance(output_bname, str):
            return f'{output_bname}-{str(self.branch)}.slcio'
        else:
            raise Exception('Either input_file must be a string, or a list while output_bname is a str')
    
    def parse_marlin_globals(self) -> str:
        globals = filter(lambda tup: tup[0] not in ['MaxRecordNumber', 'LCIOInputFiles', 'SkipNEvents'], self.globals)
        return ' '.join([f'--global.{key}="{value}"' for key, value in globals])
    
    def parse_marlin_constants(self) -> str:
        return ' '.join([f'--constant.{key}="{value}"' for key, value in self.constants])
    
    def build_command(self, **kwargs):
        steering = self.get_steering_parameters()
        
        input_files = steering['input_files']
        n_events_skip = steering['n_events_skip']
        n_events_max = steering['n_events_max']
        executable = steering['executable']
        steering_file = steering['steering_file']
        
        temp = self.get_temp_dir()
        
        cmd =  f'source $REPO_ROOT/setup.sh'
        cmd += f' && echo "Starting Marlin at $(date)"'
        cmd += f' && mkdir -p "{temp}" && cd "{temp}"'
        
        str_max_record_number = f' --global.MaxRecordNumber={str(n_events_max)}' if n_events_max is not None else ''
        str_skip_n_events = f' --global.SkipNEvents={str(n_events_skip)}' if (n_events_skip is not None and n_events_skip > 0) else ''
        
        cmd += f' && ( {executable} {steering_file} {self.parse_marlin_constants()}{self.parse_marlin_globals()}{str_max_record_number}{str_skip_n_events} --global.LCIOInputFiles="{" ".join(input_files)}" || true )'
        cmd += f' && echo "{",".join(input_files)}" >> Source.txt'
        cmd += f' && echo "Finished Marlin at $(date)"'
        cmd += f' && ( sleep 2'
        
        if self.check_output_root_ttrees is not None:
            for name, ttree in self.check_output_root_ttrees:
                cmd += f' && ( echo "Info: Checking if TTree <{ttree}> exists" && is_root_readable ./{name} {ttree} && echo "Success: TTree <{ttree}> in file <{name}> exists" ) '
                
        if self.check_output_files_exist is not None:
            for name in self.check_output_files_exist:
                cmd += f' && echo "Info: Checking if file <{name}> exists" && [ -f ./{name} ] && echo "Success: File <{name}> exists"'
        
        if self.check_output_lcio_files is not None:
            for name in self.check_output_lcio_files:
                cmd += f' && echo "Info: Checking with lcio_event_counter that file <{name}> contains events" && counts=$(lcio_event_counter {name}) && [ ! -z "$counts" ] && [ "$counts" -gt 0 ] && echo "Success: File <{name}> contains <${{counts}}> events!"'
        
        cmd += f' && mv "{self.output_file}" "{self.output()[1].path}" && cd .. && mv "{temp}" "{self.output()[0].path}" )'

        return cmd
    
    def output(self):
        return [
            self.local_directory_target(str(self.branch)),
            self.local_target(f'{self.branch}.slcio')
        ]
        
    def run(self, **kwargs):
        ShellTask.run(self, keep_cwd=True, **kwargs)

class AbstractIndex(BaseTask):
    """This task creates two indeces:
    1. samples.npy: An index of available SLCIO sample files with information about the file location, number of events, physics process and polarization
    2. processes.npy: An index containing all encountered physics processes for each polarization and their cross section-section values 
    """
    index: Optional[ProcessIndex] = None
    overview:bool = cast(bool, luigi.BoolParameter(description='Whether or not to force showing the overview when the task is already done.',
                                                   default=False))
    
    def slcio_files(self) -> list[str]:
        from analysis.configurations import zhh_configs
        config = zhh_configs.get(str(self.tag))
        if callable(config.slcio_files):
            files = config.slcio_files(self)
        elif config.slcio_files is not None:
            files = config.slcio_files
        else:
            raise Exception(f'Invalid slcio_files in config <{self.tag}>')
            
        files.sort()
        return files
    
    def output(self):
        return [
            self.local_target('processes.npy'),
            self.local_target('samples.npy'),
            self.local_target('processes.csv'),
            self.local_target('samples.csv')
        ]
    
    def run(self):
        temp_files: list[law.LocalFileTarget] = self.output()
        BaseTask.touch_parent(temp_files[0])

        self.index = index = ProcessIndex(str(temp_files[0].path), str(temp_files[1].path), self.slcio_files())
        self.index.load()
        
        # For compatability, also save as CSV
        np.savetxt(cast(str, self.output()[2].path), index.processes, delimiter=',', fmt='%s', header=','.join(cast(np.ndarray, index.processes).dtype.names))
        np.savetxt(cast(str, self.output()[3].path), index.samples, delimiter=',', fmt='%s', header=','.join(cast(np.ndarray, index.samples).dtype.names))
        
        self.publish_message(f'Loaded {len(index.samples)} samples and {len(index.processes)} processes')
        self.printOverview()
    
    def complete(self):
        complete = super().complete()
        
        if complete and self.overview:
            self.printOverview()
            
        return complete
    
    def printOverview(self):
        from law.util import colored
        from .utils import index_overview
        
        processes = np.load(str(self.output()[0].path))
        samples = np.load(str(self.output()[1].path))
        
        self.publish_message(colored(index_overview(samples, processes, self), color='green', background='black'))

class AbstractCreateChunks(BaseTask):
    """Base class for CreateChunks tasks
    Require a luigi.IntParameter jobtime for the target job runtime
    in seconds.

    Args:
        BaseTask (_type_): _description_

    Raises:
        NotImplementedError: _description_

    Returns:
        _type_: _description_
    """
    fraction:float = cast(float, luigi.FloatParameter(description='Ratio of available events (as estimated from AbstractIndex task)',
                                                      default=1.))
    
    overview:bool = cast(bool, luigi.BoolParameter(description='Whether or not to force showing the overview when the task is already done.',
                                                   default=False))
    
    # time to start up Marlin; is substracted in the RuntimeAnalysis
    T0_MARLIN:int = 10
    
    def requires(self):
        raise NotImplementedError("""requires must be implemented by an inheriting class and return at least two items with a third optional:
first a task implementing AbstractIndex
second a task implementing AbstractMarlin
[third, optional: a task implementing AbstractCreateChunks; this will be assumed to be the basis for a MarlinTask giving rise to the samples in AbstractIndex]""")
    
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def output(self):
        return [
            self.local_target('chunks.npy'),
            self.local_target('runtime_analysis.npy'),
            self.local_target('time_per_event.npy'),
            self.local_target('process_normalization.npy'),
            
            self.local_target('chunks.csv'),
            self.local_target('runtime_analysis.csv'),
            self.local_target('time_per_event.csv'),
            self.local_target('process_normalization.csv'),
        ]
    
    def run(self):
        from analysis.configurations import zhh_configs
        from zhh import get_runtime_analysis, get_process_normalization, \
                        get_adjusted_time_per_event, get_sample_chunk_splits, \
                        construct_sample_groups, CHUNK_SPLIT_MODES
        
        config = zhh_configs.get(str(self.tag))
        
        # inputs
        inputs = self.input()
        input_targets = {
            'index': inputs[0],
            'marlin': inputs[1]
        }
        
        PROCESS_INDEX = input_targets['index'][0].path
        SAMPLE_INDEX = input_targets['index'][1].path
        DATA_ROOT = osp.dirname(input_targets['marlin']['collection'][0][0].path)
        
        processes = np.load(PROCESS_INDEX)
        samples = np.load(SAMPLE_INDEX)
        
        # construct sample_groups
        sample_groups = None

        if len(inputs) > 2:
            src_chunks = np.load(inputs[2][0].path)
            sample_groups = construct_sample_groups(src_chunks, samples)
        
        runtime_analysis = get_runtime_analysis(DATA_ROOT)
        process_normalization = get_process_normalization(processes, samples,
                                                          RATIO_BY_TOTAL=self.fraction)
        time_per_event = get_adjusted_time_per_event(runtime_analysis, T0=self.T0_MARLIN)

        chunks = get_sample_chunk_splits(samples, process_normalization=process_normalization,
                    adjusted_time_per_event=time_per_event, MAXIMUM_TIME_PER_JOB=cast(int, self.jobtime),
                    custom_statistics=config.custom_statistics, sample_groups=sample_groups,
                    split_mode=CHUNK_SPLIT_MODES['ONE_TO_MANY'] if sample_groups is None else CHUNK_SPLIT_MODES['MANY_TO_MANY'])
        
        BaseTask.touch_parent(self.output()[0])
        
        np.save(str(self.output()[0].path), chunks)
        np.save(str(self.output()[1].path), runtime_analysis)
        np.save(str(self.output()[2].path), time_per_event)
        np.save(str(self.output()[3].path), process_normalization)
        
        # For compatability, also save the final results as CSV
        np.savetxt(str(self.output()[4].path), chunks, delimiter=',', fmt='%s', header=','.join(chunks.dtype.names))
        np.savetxt(str(self.output()[5].path), runtime_analysis, delimiter=',', fmt='%s', header=','.join(runtime_analysis.dtype.names))
        np.savetxt(str(self.output()[6].path), time_per_event, delimiter=',', fmt='%s', header=','.join(time_per_event.dtype.names))
        np.savetxt(str(self.output()[7].path), process_normalization, delimiter=',', fmt='%s', header=','.join(process_normalization.dtype.names))
        
        self.printOverview()
        
    def printOverview(self):
        from law.util import colored
        from .utils.task_overviews import chunk_overview
        
        chunks = np.load(str(self.output()[0].path))
        time_per_event = np.load(str(self.output()[2].path))
        process_normalization = np.load(str(self.output()[3].path))
        
        self.publish_message(colored(chunk_overview(chunks, time_per_event, process_normalization, self),
                                     color='green', background='black'))
        
    def complete(self):
        complete = super().complete()
        
        if complete and self.overview:
            self.printOverview()
        
        return complete

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
        
        return cmd
    
    def run(self, **kwargs):
        ShellTask.run(self, cwd=self.get_temp_dir(), **kwargs)