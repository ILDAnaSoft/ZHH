def deepmerge(a:dict, b:dict)->dict:
    """Merges items from dict b into dict a recursively.

    Args:
        a (dict): _description_
        b (dict): _description_

    Returns:
        _type_: dict
    """
    
    for k, v in b.items():
        if isinstance(v, dict) and k in a and isinstance(a[k], dict):
            a[k] = deepmerge(a[k], v)
        else:
            a[k] = v
            
    return a