from yaml import safe_load
from json import load
from os.path import expandvars, isfile

def replace_references(tree:dict, keys=None)->dict:
    """Traverses through the dict tree recursively and tries to replace values ending
    with .yaml/.yml and .json with the parsed contents of the respective files. Also
    parses environment variables for file candidates

    Args:
        tree (dict): _description_
        keys (_type_, optional): _description_. Defaults to None.

    Raises:
        Exception: _description_

    Returns:
        dict: _description_
    """

    if keys is None:
        keys = tree.keys()
    
    for key in keys:
        if isinstance(tree[key], dict):
            replace_references(tree[key])
        elif isinstance(tree[key], list):
            replace_references(tree[key], list(range(len(tree[key]))))
        elif isinstance(tree[key], str):
            isYAML = tree[key].endswith('.yaml') or tree[key].endswith('.yml')
            isJSON = tree[key].endswith('.json')

            if not isYAML and not isJSON:
                continue
            
            filepath = expandvars(tree[key])
            if not isfile(filepath):
                raise Exception(f'Property <{filepath}> can not be interpreted as a file')

            with open(filepath) as f:
                if isYAML:
                    tree[key] = safe_load(f)
                else:
                    tree[key] = load(f)

    return tree