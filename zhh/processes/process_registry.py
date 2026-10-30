class ProcessDefinition:
    def __init__(self, name:str, id:int):
        self.name = name
        self.id = id

    def __repr__(self):
        return f'<ProcessDefinition {self.name}:{self.id}>'

class ProcessRegistry:
    FORBIDDEN_KEYS = ['add', 'clear', 'getId', 'getName', 'getItemByName', 'getItemById', 'map', 'inverted', 'FORBIDDEN_KEYS']

    items:dict[str, ProcessDefinition] = {}

    def clear(self):
        for name in self.items:
            self.__delattr__(name)

        self.items = {}
    
    def add(self, proc:ProcessDefinition):
        if proc.name in self.items:
            print(f'Warning: Process <{proc.name}> is already registered and will be overwritten')

        if proc.name in self.FORBIDDEN_KEYS or (proc.name.startswith('__') and proc.name.endswith('__')):
            raise Exception(f'Forbidded name <{proc.name}>')

        self.items[proc.name] = proc

        # for compatability with the previous ProcessCategories implementation, one can also use ProcessCategories.<name> -> id
        self.__setattr__(proc.name, proc.id)

    def getId(self, proc_name:str)->int:
        return self.items[proc_name].id

    def getName(self, id:int)->str:
        return self.inverted[id]

    def getItemByName(self, proc_name:str)->ProcessDefinition:
            return self.items[proc_name]
    
    def getItemById(self, id:int)->ProcessDefinition:
        return self.items[self.getName(id)]

    def __repr__(self):
        items_str = '\n  ' + ('\n  '.join([ repr(item) for item in self.items.values() ])) + '\n' if len(self.items) else ''

        return f'ProcessRegistry<[{ items_str }]>'

    @property
    def inverted(self):
        return { proc.id: name for name, proc in self.items.items() }

    @property
    def map(self):
        return { name: proc.id for name, proc in self.items.items() }

GlobalProcessRegistry = ProcessRegistry()
"""Stores all available processes
"""