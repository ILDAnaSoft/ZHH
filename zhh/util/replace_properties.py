def replace_properties(tree:dict, globals:dict, keys=None)->dict:
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

    if keys is None:
        keys = tree.keys()
    
    for key in keys:
        if isinstance(tree[key], dict):
            replace_properties(tree[key], globals)
        elif isinstance(tree[key], list):
            replace_properties(tree[key], globals, list(range(len(tree[key]))))
        elif isinstance(tree[key], str):
            if not '.' in tree[key]:
                continue

            parts = tree[key].split('.')
            if len(parts) != 2:
                continue

            prefix, suffix = parts
            
            if prefix in globals:
                if hasattr(globals[prefix], suffix):
                    tree[key] = getattr(globals[prefix], suffix)
                else:
                    tree[key] = globals[prefix][suffix]

    return tree