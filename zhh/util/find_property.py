def find_property(parent, property:str|list[str]):
    value = parent

    if isinstance(property, str):
        property = '.'.split(property)

    for key in property:
        if hasattr(value, key):
            value = getattr(value, key)
        elif key in value:
            value = value[key]
        else:
            raise Exception('Could not resolve value')
    
    return value