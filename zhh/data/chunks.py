from tqdm.auto import tqdm
from typing import List
import numpy as np

def join_chunks(size:int, files:List[str]):    
    result = None
    pointer = 0

    for file in tqdm(files):
        part = np.load(file)
        if result is None:
            result = np.zeros(size, dtype=part.dtype)
            
        result[pointer:(pointer+len(part))] = part
        pointer += len(part)
    
    return result