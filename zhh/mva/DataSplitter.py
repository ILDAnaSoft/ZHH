import numpy as np, uproot as ur
from math import floor

class DataSplitter:
    def __init__(self, *fracs:float, name:str='data', split_names:list[str]=['train', 'test', 'val']):
        """Class to split dictonaries of { key: np.ndarray[n,,..] } structure
        into multiple dictionaries of randomly selected subsets for each key 

        Args:
            name (str, optional): _description_. Defaults to 'data'.
            split_names (list[str], optional): _description_. Defaults to ['train', 'test', 'val'].
        """
        
        fracs_np = np.array(fracs)
        assert(fracs_np.sum() < 1)
        
        fractions = np.zeros(len(fracs_np) + 1)
        fractions[:-1] = fracs_np
        fractions[-1] = 1 - fracs_np.sum()
        
        self._fractions = fractions
        self._name = name
        self._split_names = split_names
        
    def split(self, items:dict[str, np.ndarray], seed:int=42)->list[dict[str, np.ndarray]]:
        
        sizes = np.array([len(items[item]) for item in items], dtype=int)
        size = sizes[0]
        fractions = self._fractions
        
        if not ((sizes == size).all()):
            raise Exception('All items must be of the same size')
        
        shuffled_indices = np.arange(size)

        np.random.seed(seed)
        np.random.shuffle(shuffled_indices)
        
        n_groups = len(fractions)
        print(len(shuffled_indices))
        idx_mask = np.zeros(len(shuffled_indices), dtype='?')
        
        data = []
        
        for i, fraction in enumerate(fractions):
            split_name = self._split_names[i]
            idx_mask[:] = 0
            
            if i == 0: # first
                idx_mask[:floor(fraction * size)] = True
            elif i == (n_groups - 1): # last
                idx_mask[floor(fractions[i - 1] * size):] = True
            else:
                idx_mask[floor(fractions[i - 1] * size):floor(fraction * size)] = True

            chunk = {}

            for name in items:
                item = items[name]
                chunk[name] = item[idx_mask]
                
            data.append(chunk)
        
        return data
    
    def load(self, path:str)->dict[str, np.ndarray]:
        result = {}
        
        with ur.open(path) as rf:
            tree = rf['data']
            for key in tree.keys():
                result[key] = np.array(tree[key].array())
        
        return result
        
    def store(self, data:dict[str, np.ndarray]):
        for i, split_name in enumerate(self._split_names):
        
            with ur.recreate(f'{self._name}_{split_name}.root', compression=ur.ZSTD(5)) as rf:
                rf['data'] = data

ds = DataSplitter(0.3)