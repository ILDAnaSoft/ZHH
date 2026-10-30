from typing import Mapping, Any, TypeVar

T = TypeVar('T', bound=Mapping[str, Any]|list)

def replace_properties(tree:T, globals:Mapping[str, Any], keys=None)->T:
    """Traverses through the dict tree recursively and attempts to replace values strings
    with one dot with a value from globals. The first key will be interpreted as the key
    in globals, the second as a property of that value.

    Args:
        tree (dict): _description_
        globals (dict): _description_
        keys (_type_, optional): _description_. Defaults to None.

    Returns:
        dict: _description_
    """

    node:Any = tree

    if keys is None:
        keys = node.keys()
    
    for key in keys:
        if isinstance(node[key], dict):
            replace_properties(node[key], globals)
        elif isinstance(node[key], list):
            replace_properties(node[key], globals, list(range(len(node[key]))))
        elif isinstance(node[key], str):
            if not '.' in node[key]:
                continue

            parts = node[key].split('.')
            if len(parts) != 2:
                continue

            prefix, suffix = parts
            
            if prefix in globals:
                if hasattr(globals[prefix], suffix):
                    node[key] = getattr(globals[prefix], suffix)
                else:
                    try:
                        if suffix in globals[prefix]:
                            node[key] = globals[prefix][suffix]
                        else:
                            print(f'Key <{suffix}> not found in {globals[prefix]}')
                            raise KeyError(f'Key {suffix} not found')
                    except Exception as e:
                        print(globals, prefix)
                        raise KeyError(f'Could not access key <{suffix}> in {globals[prefix]}')

    return tree