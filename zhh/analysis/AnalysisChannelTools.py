from .AnalysisChannel import AnalysisChannel
from math import floor
import numpy as np

def apply_split(*fracs, source:AnalysisChannel, name:str='split', seed:int=42)->np.ndarray:
    """Creates an array of labels with len(fracs)+1 different values.
    In the easiest way, given only one fraction, will only assign 0 to
    the fraction given and 1 to the remainder. Assigns the resulting
    array to the column given by name in the datasource store.

    Args:
        source (AnalysisChannel): _description_
        name (str, optional): _description_. Defaults to 'split'.
        seed (int, optional): _description_. Defaults to 42.

    Returns:
        np.ndarray: _description_
    """
    
    np_fracs = np.array(fracs)
    fractions = np.zeros(len(np_fracs) + 1)
    fractions[:-1] = np_fracs
    fractions[-1] = 1 - np_fracs.sum()
        
    store = source.getStore()
    size = len(source)
    shuffled_indices = np.arange(size)
    
    rng = np.random.default_rng(seed)
    rng.shuffle(shuffled_indices)
    
    n_groups = len(fractions)
    split = np.zeros(size, dtype='B')
    
    for i, fraction in enumerate(fractions):
        if i == 0: # first
            split[shuffled_indices[:floor(fraction * size)]] = i
        elif i == (n_groups - 1): # last
            split[shuffled_indices[floor(fractions[i - 1] * size):]] = i
        else:
            split[shuffled_indices[floor(fractions[i - 1] * size):floor(fraction * size)]] = i
    
    store[name] = split
    
    return split
    
def mod_weights_from_split(source:AnalysisChannel, split_prop:str='split', weight_prop:str='weight'):
    """Creates a new array of weights in which the sum of the weights of each
    split category equals the whole sum of weights. Useful for plotting when
    running inference on test/valdiation data.

    Args:
        source (AnalysisChannel): _description_
        split_prop (str, optional): _description_. Defaults to 'split'.
        weight_prop (str, optional): _description_. Defaults to 'weight'.

    Returns:
        _type_: _description_
    """
    
    store = source.getStore()
    split = store[split_prop]
    weight = store[weight_prop]

    weights_new = np.zeros(len(store))

    for tag in np.unique(split):
        subset = weight[split == tag]
        weights_new[split == tag] = subset * weight.sum()/subset.sum()
        
    return weights_new
