def merge(source:dict, destination:dict)->dict:
    for key, value in source.items():
        if isinstance(value, dict):
            node = destination.setdefault(key, {})
            merge(value, node)
        else:
            destination[key] = value
    
    return destination

class OptionDict():
    properties:dict = {}
    
    def __init__(self, properties:list[tuple[str, str|int|float]]=[]):
        """Helper class for constructing dicts with
        predefined properties and merging of inputs

        Args:
            properties (list[tuple[str, str | int | float]], optional): input properties as a list of tuples of (key, value) structure. Defaults to [].
        """
        for key, value in properties:
            self.addProperty(key, value)
    
    def addProperty(self, prop, default):
        self.properties[prop] = default
    
    def default(self):
        res = {}
        for prop in self.properties:
            res[prop] = self.properties[prop]
        
        return res
    
    def merge(self, values:dict={}):
        res = self.default()
        return merge(values, res)