class Process:
    process_id: int
    process_name: str
    files: list[str]
    pol_electron: float
    pol_positron: float
    cross_section: float
    cross_section_error: float
    
    def __init__(self) -> None:
        pass
    
    