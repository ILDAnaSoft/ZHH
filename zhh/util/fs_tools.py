import os

def dirs_inside(path:str)->list[str]:
    return [ f.path for f in os.scandir(path) if f.is_dir() ]

def files_inside(path:str)->list[str]:
    return [ f.path for f in os.scandir(path) if f.is_file() ]