if __name__ == "__main__":
    import json, os
    import os.path as osp
    import numpy as np
    from tqdm.auto import tqdm
    from zhh import get_pol_key, get_raw_files, is_readable, ProcessIndex
    from pyLCIO import IOIMPL

    PROCESS_INDEX = '/afs/desy.de/user/b/bliewert/nfs/zhh/CreateRawIndex/v1/processes.npy'
    SAMPLE_INDEX  = '/afs/desy.de/user/b/bliewert/nfs/zhh/CreateRawIndex/v1/samples.npy'
    RAW_FILE_LIST = list(get_raw_files())

    a = ProcessIndex(PROCESS_INDEX, SAMPLE_INDEX, RAW_FILE_LIST)
    a.load()