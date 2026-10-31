from collections.abc import Callable
from typing import TYPE_CHECKING
import numpy as np

if TYPE_CHECKING:
    from ..analysis.DataSource import DataSource, FinalStateCounts

# (source, final state counts) -> mask of events belonging to an event category
FinalStateDefinition = Callable[['DataSource', 'FinalStateCounts'], np.ndarray]

class EventCategoryDefinition:
    def __init__(self, name:str, id:int,
                 final_state:'FinalStateDefinition|None'=None,
                 latex_label:str|None=None):
                 
        self.name = name
        self.id = id
        self.final_state = final_state
        self.latex_label = latex_label

    def __repr__(self):
        return f'<EventCategoryDefinition {self.name}:{self.id} [finalState={"Registered" if self.final_state is not None else "Unknown"}, latexLabel={self.latex_label}]>'

class EventCategoryRegistry:
    FORBIDDEN_KEYS = ['add', 'clear', 'getId', 'getName', 'getItemByName', 'getItemById', 'map', 'inverted', 'FORBIDDEN_KEYS']

    items:dict[str, EventCategoryDefinition] = {}
    
    def clear(self):
        for name in self.items:
            self.__delattr__(name)

        self.items = {}

    def add(self, definition:EventCategoryDefinition):
        if definition.name in self.items:
            print(f'Warning: Category <{definition.name}> is already registered and will be overwritten')

        if definition.name in self.FORBIDDEN_KEYS or (definition.name.startswith('__') and definition.name.endswith('__')):
            raise Exception(f'Forbidded name <{definition.name}>')
        
        self.items[definition.name] = definition

        # for compatability with the previous EventCategories implementation, one can also use EventCategories.<name> -> id
        self.__setattr__(definition.name, definition.id)

    def getId(self, proc_name:str)->int:
        return self.items[proc_name].id

    def getName(self, id:int)->str:
        return self.inverted[id]

    def getItemByName(self, proc_name:str)->EventCategoryDefinition:
            return self.items[proc_name]
    
    def getItemById(self, id:int)->EventCategoryDefinition:
        return self.items[self.getName(id)]
    
    def __repr__(self):
        items_str = '\n  ' + ('\n  '.join([ repr(item) for item in self.items.values() ])) + '\n' if len(self.items) else ''

        return f'EventCategoryRegistry<[{ items_str }]>'

    @property
    def inverted(self)->dict[int, str]:
        return { proc.id: name for name, proc in self.items.items() }

    @property
    def map(self)->dict[str, int]:
        return { name: proc.id for name, proc in self.items.items() }

GlobalEventCategoryRegistry = EventCategoryRegistry()
"""Stores all available event categories
"""