import numpy as np
import uproot as ur
from math import ceil

def numpy2root(data:np.ndarray, root_file:str, tree_name:str, bufferwidth:int=10000):
    """Creates a structured numpy array to a ROOT TTree and stores it in a file.
    Data is read in chunks of size bufferwidth*n_col(data).

    Args:
        data (np.ndarray): The input np structured array.
        root_file (str): The output ROOT file (will be overwritten if it exists).
        tree_name (str): Name of the TTree to be created.
        bufferwidth (int, optional): _description_. Defaults to 10000.
    """
    
    mktree_arg = {}
    for name in data.dtype.fields:
        dtype = data.dtype.fields[name][0]
        mktree_arg[name] = dtype

    with ur.recreate(root_file) as rf:
        rf.mktree(tree_name, mktree_arg, title=tree_name)
        rf[tree_name].show()
        
        extend_obj = {}
        
        pointer = 0
        for i in range(ceil(len(data)/bufferwidth)):
            for name in data.dtype.names:
                extend_obj[name] = data[name][pointer:(pointer+bufferwidth)]
            
            rf[tree_name].extend(extend_obj)
            
            pointer += bufferwidth