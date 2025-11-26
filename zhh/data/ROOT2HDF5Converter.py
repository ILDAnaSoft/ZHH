import functools, os.path as osp
import awkward as ak
import numpy as np
import h5py
import uproot as ur
from os import makedirs
from typing import TYPE_CHECKING, cast
from multiprocessing import Pool, cpu_count
from tqdm.auto import tqdm
from math import ceil
from ..task.AbstractTask import AbstractTask

ChunkedConversionResult = tuple[int, tuple[tuple[int, int]|tuple[int], str]]

# for creating ROOT dicts
# if not TYPE_CHECKING:
# ROOT.gInterpreter.GenerateDictionary("ROOT::VecOps::RVec<vector<double>>", "vector;ROOT/RVec.hxx")
# ROOT.gInterpreter.GenerateDictionary("ROOT::VecOps::RVec<ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double>>>", "vector;ROOT/RVec.hxx;Math/Vector4D.h")

def create_chunks(tree:str, branch:str, root_files:list[str], out_bname:str, chunk_size:int=256,
                  overwrite_if_exists:bool=True, read_size:int=16, dtype:int|None=None)->list[tuple[int, str, str, list[str], str, bool, int, str]]:

    chunks = []
    chunk_idx = 0
    for i in range(0, len(root_files), chunk_size):
        chunks.append((chunk_idx, tree, branch, root_files[i:i+chunk_size], f'{out_bname}.{chunk_idx}.h5', overwrite_if_exists, read_size, dtype))
        chunk_idx += 1
    
    return chunks

class ROOT2HDF5Converter:
    def __init__(self, root_files:list[str], output_file:str, tree:str, branch:str,
                 output_bname:str|None=None, output_name:str|None=None, dtype:str|None=None):
        """_summary_

        Args:
            root_files (list[str]): list of paths to input ROOT files
            output_file (str): output HDF5 file
            tree (str): name of TTree
            branch (str): name of branch in TTree
            output_bname (str | None, optional): basename of HDF5 file. Defaults to None.
            output_name (str | None, optional): name of virtual dataset to create in output_file.
                Will use branch if None. Defaults to None.
            dtype (str | None, optional): _description_. Defaults to None.
        """
    
        assert(output_file.lower().endswith('.h5') or output_file.lower().endswith('.hdf5'))

        if output_bname is None:
            output_bname = f'{osp.dirname(output_file)}/items/{tree}.{branch}/item'

        if output_name is None:
            output_name = branch
        
        self._root_files = root_files
        self._vds_file = output_file
        
        self._tree = tree
        self._branch = branch
    
        self._output_bname = output_bname
        self._output_name = output_name
        self._dtype = dtype

        if not osp.isdir(osp.dirname(output_bname)):
            makedirs(osp.dirname(output_bname), exist_ok=True)
    
    def getChunks(self, **kwargs):
        return create_chunks(self._tree, self._branch, self._root_files, self._output_bname, **kwargs)
    
    def checkExisting(self, chunks:list)->tuple[bool, list[int], list[int]]:
        already_done = False
        sizes = []
        ncols_found = 0
        nrows_found = 0

        # check output file of first chunk
        if osp.isfile(chunks[0][4]):
            already_done = True

            for i, chunk in enumerate(chunks):
                if not already_done:
                    break

                chunk_idx = chunk[0]
                chunk_files = chunk[3]
                out_file = chunk[4]

                if osp.isfile(out_file):
                    with h5py.File(out_file) as hf:
                        if not 'shape' in hf:
                            print(f'File found at {out_file} is corrupted. Existing files for Tree:Branch={chunks[0][1]}:{chunks[0][2]} considered missing')
                            already_done = False
                            break

                        shape = np.array(hf['shape'], dtype=int)
                        sizes.append(shape[0])
                        ncols_found = 1 if len(shape) == 1 else shape[1]

                        if i == 0:
                            ncols = ncols_found
                            if self._dtype is None:
                                self._dtype = str(cast(h5py.Dataset, hf['dim0']).dtype)
                        else:
                            assert(ncols == ncols_found)

                        already_done = already_done and cast(bool, 
                            hf.attrs['chunk_idx'] == chunk_idx and
                            hf.attrs['tree'] == self._tree and
                            hf.attrs['branch'] == self._branch and
                            np.all(hf.attrs['input_files'] == chunk_files))
                        
                        if not already_done:
                            print('chunk_idx=', hf.attrs['chunk_idx'], chunk_idx)
                            print('tree=', hf.attrs['tree'], self._tree)
                            print('branch=', hf.attrs['branch'], self._branch)
                            print('input_files=', hf.attrs['input_files'], chunk_files)

                            raise Exception(f'File <{out_file}> for chunk <{chunk_idx}> does not fit to expected '+
                                            'data structure. See above print for property=<found> <expected>')
                else:
                    already_done = False
                    raise Exception(f'File <{out_file}> for chunk <{chunk_idx}> does not exist.'+
                                    ' Please delete all chunks to make sure everything is consistent')

                #chunk_idx, tree, branch, root_files[i:i+chunk_size], f'{out_bname}.{chunk_idx}.h5', overwrite_if_exists, read_size, dtype = chunk
                #fpath = f'{self._output_bname}.{chunk_idx}.h5'

            nrows_found = int(np.sum(sizes))
        
        shape = [nrows_found] if ncols_found < 2 else [nrows_found, ncols_found]

        return (already_done, shape, sizes)

    def convertLazy(self, nrows:int|None=None, check_existing:bool=False, **kwargs)->tuple[AbstractTask, list[AbstractTask]]:
        """Returns one or multiple tasks which represent the ROOT->HDF5 conversion
        See ProcessRunner for a tool to execute them.

        Args:
            nrows (int | None, optional): _description_. Defaults to None.
            check_existing (bool, optional): _description_. Defaults to False.

        Returns:
            AbstractTask: _description_
        """

        chunks = self.getChunks(**kwargs)

        done = False
        ncols = 1
        sizes = []

        # check if potentially existing chunks are valid
        if check_existing:
            done, shape, sizes = self.checkExisting(chunks)
            ncols = shape[1] if len(shape) == 2 else shape[0]
        else:
            print(f'No existing (first) chunk found for Tree:Branch <{self._tree}:{self._branch}>. '+
                  f'Proceeding with conversion...')

        h5_files = [chunk[4] for chunk in chunks]
        conversion_tasks = [] if done else [AbstractTask(f'ROOT2HDF5Task:{self._tree}.{self._branch}', per_chunk, (chunk, )) for chunk in chunks]
        create_vds_task = CreateVDSTask(f'CreateVDS:{self._tree}.{self._branch}',
                                        args=(h5_files, self._vds_file, self._output_name, self._dtype, done, sizes, ncols))
        create_vds_task.requires('conversion', conversion_tasks)

        return create_vds_task, conversion_tasks

    def convert(self, nrows:int|None=None, check_existing:bool=False, **kwargs):
        """_summary_

        Args:
            nrows (int | None, optional): _description_. Defaults to None.
            check_existing (bool, optional): _description_. Defaults to False.

        Raises:
            Exception: _description_
        """

        chunks = self.getChunks(**kwargs)

        done = False
        nrows_found = 0
        ncols = 1
        sizes = []

        # check if potentially existing chunks are valid
        if check_existing:
            done, shape, sizes = self.checkExisting(chunks)
            ncols = shape[1] if len(shape) == 2 else shape[0]
            nrows_found = int(np.sum(sizes))
        else:
            print(f'No existing (first) chunk found for Tree:Branch <{self._tree}:{self._branch}>. Proceeding with conversion...')

        if not done:
            conv_result = process_chunks(chunks, n_files=len(self._root_files))

            first_entry = conv_result[0][1]
            first_shape = first_entry[0]

            if self._dtype is None:
                self._dtype = first_entry[1]

            sizes = [c[1][0][0] for c in conv_result] # get number of rows for each item in result
            nrows_found = np.sum(sizes)
            if nrows is not None and nrows_found != nrows:
                raise Exception(f'Size mismatch: Found nrows_found={nrows_found} rows, but expected {nrows}')

            assert(len(first_shape) <= 2)

            ncols = first_shape[1] if len(first_shape) == 2 else 1
            shape = (nrows_found,) if len(first_shape) == 1 else (nrows_found, ncols)

            #print(shape)
            #is_1d = 
            #h5_files = sorted(glob(f'/data/dust/user/bliewert/zhh/buffer_test/item.{TREE}.{BRANCH}.*h5'), key=lambda x: int(x.split('.')[-2]))
        
        h5_files = [chunk[4] for chunk in chunks]
        
        createVDS(h5_files, self._vds_file, self._output_name, nrows=nrows_found, ncols=ncols, dtype=self._dtype, sizes=sizes)

def createVDS(h5_files:list[str], output_file:str, output_name:str, nrows:int, ncols:int=1, dtype:str|None=None, sizes:list[int]|None=None):
    """_summary_

    Args:
        h5_files (list[str]): _description_
        output_file (str): _description_
        output_name (str): _description_. Defaults to None.
        nrows (int): _description_
        ncols (int, optional): _description_. Defaults to 1.
        dtype (str | None, optional): _description_. Defaults to None.
        sizes (list[int] | None, optional): _description_. Defaults to None.
    """
    if dtype is None:
        with h5py.File(h5_files[0], 'r') as hf:
            dtype = cast(h5py.Dataset, hf['dim0']).dtype

    if sizes is None:
        sizes = []
        for p in h5_files:
            with h5py.File(p, 'r') as hf:
                sizes.append(len(cast(h5py.Dataset, hf['dim0'])))

    with h5py.File(output_file, 'a') as hf:
        for col in range(ncols):
            layout = h5py.VirtualLayout((nrows, ), dtype)

            counter = 0

            for i, path in enumerate(h5_files):
                ncur = sizes[i]
                vsource = h5py.VirtualSource(path, f'dim{col}', shape=(ncur, ), dtype=dtype)
                layout[counter:(counter+ncur)] = vsource
                counter += ncur

            # Add virtual dataset to output file
            hf.create_virtual_dataset(f'{output_name}.dim{col}' if ncols > 1 else output_name, layout, fillvalue=np.nan)

    return True

class CreateVDSTask(AbstractTask):
    def work(self, h5_files:list[str], vds_file:str, output_name:str, dtype:str|None, done:bool, sizes:list[int], ncols:int, **kwargs):
        if not done:
            conversion:list[ChunkedConversionResult] = kwargs['conversion']

            first_entry = conversion[0][1]
            first_shape = first_entry[0]
            first_dtype = first_entry[1]

            dtype = first_dtype if dtype is None else dtype

            sizes = [c[1][0][0] for c in conversion]        
            ncols = first_shape[1] if len(first_shape) == 2 else 1

        #return True
        return createVDS(h5_files, vds_file, output_name, nrows=np.sum(sizes), ncols=ncols, dtype=dtype, sizes=sizes)

def tree_n_rows(sources:list[str], tree:str, use_uproot:bool=True, use_mp:bool=True)->int:
    nrows = 0

    if use_mp and len(sources) > 4 * cpu_count():
        with Pool() as pool:
            chunks = []
            chunk_size = ceil(len(sources) / cpu_count())
            for i in range(0, len(sources), chunk_size):
                chunks.append(sources[i:i + chunk_size])

            #print(len(sources), cpu_count(), chunk_size)
            #print([len(c) for c in chunks])

            progress = tqdm(range(len(chunks)))
            progress.set_description(f'Fetching size of TTree <{tree}> in <{len(sources)}> files using <{cpu_count()}> cores...')
        
            for chunk_output in pool.imap_unordered(functools.partial(tree_n_rows, tree=tree, use_uproot=use_uproot, use_mp=False), chunks):
                #print(nrows, chunk_output, nrows + chunk_output)
                nrows += chunk_output
                progress.update(1)

        return nrows
    else:
        if not TYPE_CHECKING:
            if use_uproot:
                for s in sources:
                    with ur.open(s) as uf:
                        nrows += uf[tree].num_entries
            else:
                import ROOT

                chain = ROOT.TChain(tree)

                for file in sources:
                    chain.Add(file)

                nrows = chain.GetEntries()
        
        return nrows

def translate_item(sources:list[str], tree:str, names:str|list[str], use_uproot:bool=True)->list[ak.Array]:
    result:list[ak.Array] = []

    if not TYPE_CHECKING:
        if use_uproot:
            import uproot as ur        

            for f in sources:
                with ur.open(f) as rf:
                    result.append(rf[f'{tree}/{names}'].array())            
        else:
            import ROOT

            chain = ROOT.TChain(tree)

            for file in sources:
                chain.Add(file)

            result.append(ak.from_rdataframe(ROOT.RDataFrame(chain), columns=names))

    return result

def translate_item_lazy(sources:list[str], tree:str, names:str|list[str], n_per_iter:int, use_uproot:bool=True):
    from math import ceil
    
    maxiter = ceil(len(sources) / n_per_iter)
    counter = 0

    for niter in range(maxiter):
        size = n_per_iter if (niter + 1 < maxiter) else (len(sources) - counter)
        yield translate_item(sources[counter:counter+size], tree, names, use_uproot)
        counter += size

def per_chunk(args:tuple[int, str, str, list[str], str, bool, int|None, str|None])->ChunkedConversionResult:
    """Attempts to read tree/branch from all ROOT files in file_paths.
    Depending on the value of outp_or_None:
    1. None -> (chunk_idx, result=list[ak.array]) will be returned
    2. str -> the value will be interpreted as path for a HDF5 file
        that will be written with the concatenated value of result
        under the dataset named data. return value: (chunk_idx,
        total_shape:tuple[int,int])
    Values must be of regular shape. Supports 1D and 2D TTree branches.

    Args:
        args[0] = chunk_idx (int): _description_
        args[1] = tree (str): _description_
        args[2] = branch (str): _description_
        args[3] = file_paths (list[str]): _description_
        args[4] = outp (str): _description_
        args[5] = overwrite_if_exists (bool): whether or not to
            overwrite an existing file, if outp is a str.
            ignored if outp_or_None is None
        args[6] = read_size (int|None): how many files should be
            loaded into memory at a time.
        args[7] = dtype (str|None): if None, dtype will be infer-
            red using uproot
            
    Returns:
        _type_: _description_
    """

    chunk_idx:int = args[0]
    tree:str = args[1]
    branch:str = args[2]
    file_paths:list[str] = args[3]
    output_file:str = args[4]
    overwrite_if_exists:bool = args[5]
    read_size:int|None = args[6]
    dtype:str|None = args[7]

    import h5py

    # load from HDF5
    if osp.isfile(output_file) and not overwrite_if_exists:
        with h5py.File(output_file, 'r') as hf:
            shape = tuple(cast(h5py.Dataset, hf['shape'])[:])
            dtype_read = str(cast(h5py.Dataset, hf['dim0']).dtype)

            if dtype is not None and dtype != dtype_read:
                raise Exception(f'dtype mismatch: expected <{dtype}> but found <{dtype_read}>')

        return (chunk_idx, (shape, dtype_read))
    
    # convert from ROOT files
    translated = translate_item_lazy(file_paths, tree, branch, n_per_iter=read_size) if read_size is not None else [translate_item(file_paths, tree, branch)]
    nrows = 0
    ncols = 0
    is_1d = False
    is_2d = False

    with h5py.File(output_file, 'w') as hf:
        hf.attrs['chunk_idx'] = chunk_idx
        hf.attrs['tree'] = tree
        hf.attrs['branch'] = branch
        hf.attrs['input_files'] = file_paths

        for i_result, result in enumerate(translated):
            is_1d = result[0].ndim == 1
            is_2d = result[0].ndim == 2

            # regularize
            if is_2d:
                for i in range(len(result)):
                    result[i] = ak.to_regular(result[i])

            size = sum([len(result[i]) for i in range(len(result))])

            if i_result == 0:
                # infer nrows, and ncols+dtype from first entry
                ncols = 1 if is_1d else result[0].type.content.size
                if dtype is None:
                    dtype = str(result[0].type.content) if is_1d else str(result[0].type.content.content)

                assert(is_1d or is_2d)

                for col in range(ncols):
                    # chunk-size 16MB
                    hf.create_dataset(f'dim{col}', shape=size, maxshape=(None, ), dtype=dtype, chunks=True, rdcc_nbytes=16*1024**2, fillvalue=np.nan)

            for col in range(ncols):
                if i_result:
                    cast(h5py.Dataset, hf[f'dim{col}']).resize((nrows+size, ))
                
                counter = 0
                for arr in result:
                    arr_size = len(arr)
                    dataset:h5py.Dataset = cast(h5py.Dataset, hf[f'dim{col}'])
                    dataset[(nrows+counter):(nrows+counter+arr_size)] = arr[:, col] if is_2d else arr
                    counter += arr_size

                assert(counter == size)

            nrows += size

        hf['shape'] = np.array([nrows] if is_1d else [nrows, ncols], dtype=int)
    
    assert(isinstance(dtype, str))

    return (chunk_idx, ((nrows, ncols) if is_2d else (nrows, ), dtype))

def process_chunks(chunks, n_files:int|None=None):
    chunk_outputs = []

    with Pool() as pool:
        progress = tqdm(range(n_files if n_files is not None else len(chunks)))
        
        for chunk_output in pool.imap_unordered(per_chunk, chunks):
            chunk_idx, chunk = chunk_output
            
            progress.update(len(chunks[chunk_idx][3]) if n_files is not None else 1)
            progress.set_description(f'Receiving data for chunk {chunk_idx}')
            
            chunk_outputs.append(chunk_output)
                    
            #self.save()
            
    chunk_outputs.sort(key=lambda x: x[0]) # Sort by file location

    return chunk_outputs