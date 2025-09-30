from glob import glob
from os.path import expandvars

def glob_exp(x, **kwargs):
    return glob(expandvars(x), **kwargs)