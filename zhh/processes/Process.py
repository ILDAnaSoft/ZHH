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
    
    def __str__(self):
        return f'Process <{self.process_id}:{self.process_name}>'


class MarlinResult:
    process: Process
    run_id: int
    n_events: int
    
    final_state_meta_path: str
    final_state_path: str
    
    zhh_presel_llhh_path: str
    zhh_presel_vvhh_path: str
    zhh_presel_qqhh_path: str
    
    def __init__(self,
                 process:Process,
                 run_id:int,
                 n_events:int,
                 
                 final_state_meta_path:str,
                 final_state_path: str,
                 
                 zhh_presel_llhh_path:str,
                 zhh_presel_vvhh_path:str,
                 zhh_presel_qqhh_path:str) -> None:
        
        self.process = process
        self.run_id = run_id
        self.n_events = n_events
        
        self.final_state_meta_path = final_state_meta_path
        self.final_state_path = final_state_path
        
        self.zhh_presel_llhh_path = zhh_presel_llhh_path
        self.zhh_presel_vvhh_path = zhh_presel_vvhh_path
        self.zhh_presel_qqhh_path = zhh_presel_qqhh_path


class HydratedProcess(Process):
    results: list[MarlinResult]
    map: dict[str, MarlinResult]
    
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.results = []
        self.map = {}
        
    def add_result(self, result:MarlinResult)->bool:
        id = f'{result.process.process_id}:{result.run_id}'
        if not (id in self.map):
            self.map[id] = result
            self.results.append(result)
            return True
        else:
            return False
        
    def add_results(self, results:list[MarlinResult]):
        for result in results:
            self.add_result(result)