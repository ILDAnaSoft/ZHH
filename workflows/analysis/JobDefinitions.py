from typing import Callable, Union

class JobDefinition:
    name:str
    get_raw_files:Union[list[str], Callable]

class JobDefinitionRegistry:
    definitions:dict[str,JobDefinition] = {}