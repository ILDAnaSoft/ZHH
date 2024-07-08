class Process:
    process_id: int
    process_name: str
    pol_electron: float
    pol_positron: float
    cross_section: float
    cross_section_err: float
    
    def __init__(self,
                 process_id:int,
                 process_name:str,
                 pol_electron:float,
                 pol_positron:float,
                 cross_section:float,
                 cross_section_err:float) -> None:
        
        self.process_id = process_id
        self.process_name = process_name
        self.pol_electron = pol_electron
        self.pol_positron = pol_positron
        self.cross_section = cross_section
        self.cross_section_err = cross_section_err


class MarlinResult:
    process: Process
    run_id: int
    n_events: int
    file_path: str
    
    def __init__(self,
                 process:Process,
                 run_id:int,
                 n_events:int,
                 file_path:str) -> None:
        
        self.process = process
        self.run_id = run_id
        self.n_events = n_events
        self.file_path = file_path



