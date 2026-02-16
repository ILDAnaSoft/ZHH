from collections.abc import Sequence

def find_by(items:Sequence[dict], property:str, value, is_dict:bool|None=None):
    if is_dict is None:
        is_dict = len(items) > 0 and isinstance(items[0], dict)
    
    if is_dict:
        for item in items:
            if property in item:
                if item[property] == value:
                    return item
                else:
                    raise KeyError(f'Item {property} not found')
    else:
        for item in items:
            if hasattr(item, property):
                if getattr(item, property) == value:
                    return item
            else:
                raise KeyError(f'Item {property} not found')

    raise KeyError(f'No item with {property}={value} not found')