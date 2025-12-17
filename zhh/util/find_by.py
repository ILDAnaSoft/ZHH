from collections.abc import Sequence

def find_by(items:Sequence[dict], property:str, value, is_dict:bool|None=None):
    if is_dict is None:
        is_dict = len(items) > 0 and isinstance(items[0], dict)
    
    if is_dict:
        for item in items:
            if item[property] == value:
                return item
    else:
        for item in items:
            if getattr(item, property) == value:
                return item
    
    raise KeyError(f'Item {property} not found')