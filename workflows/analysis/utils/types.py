from typing import TypedDict

SGVOptions = dict[str, str|int|float]

class WhizardOption(TypedDict):
    process_name: str
    process_definition: str
    template_dir: str
    sindarin_file: str
    iters_per_polarization:dict[str, int]|None