import functools, os.path as osp
import numpy as np
import h5py
import uproot as ur
from os import makedirs
from typing import TYPE_CHECKING, cast
from collections.abc import Sequence
from multiprocessing import Pool, cpu_count
from tqdm.auto import tqdm
from math import ceil
from ..task.AbstractTask import AbstractTask

ChunkedConversionResult = tuple[int, tuple[tuple|tuple[int], str]]

# (tree, branch, output_file, overwrite_if_exists, dtype, clamp, nan_to, keep_dim)
GroupedItemSpec = tuple[str, str, str, bool, str|None, tuple[float|int|None, float|int|None], float|int|None, bool]
GroupedChunkResult = tuple[int, dict[int, tuple[tuple, str]]]

if TYPE_CHECKING:
    import awkward as ak

# for creating ROOT dicts
# if not TYPE_CHECKING:
# ROOT.gInterpreter.GenerateDictionary("ROOT::VecOps::RVec<vector<double>>", "vector;ROOT/RVec.hxx")
# ROOT.gInterpreter.GenerateDictionary("ROOT::VecOps::RVec<ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double>>>", "vector;ROOT/RVec.hxx;Math/Vector4D.h")

def create_chunks(tree:str, branch:str, root_files:list[str], out_bname:str, clamp:tuple[float|int|None, float|int|None], nan_to:float|int|None, chunk_size:int=256,
                  overwrite_if_exists:bool=True, read_size:int=16, dtype:int|None=None, keep_dim:bool=False)->list[tuple[int, str, str, list[str], str, bool, int, str, bool]]:

    chunks = []
    chunk_idx = 0
    for i in range(0, len(root_files), chunk_size):
        chunks.append((chunk_idx, tree, branch, root_files[i:i+chunk_size], f'{out_bname}.{chunk_idx}.h5', overwrite_if_exists, read_size, dtype, clamp, nan_to, keep_dim))
        chunk_idx += 1
    
    return chunks

def detect_existing_chunk_size(out_bname:str)->int|None:
    """Returns the number of ROOT files that went into the first chunk of an item which
    has been converted in a previous run, or None if no (readable) chunk exists yet.

    The chunk layout of an existing item must not change between runs, as checkExisting()
    compares the list of input files of each chunk file against the expected one.

    Args:
        out_bname (str): basename of the chunk files, i.e. chunk i is at f'{out_bname}.{i}.h5'

    Returns:
        int|None: number of ROOT files per chunk, if it could be determined
    """

    first_chunk = f'{out_bname}.0.h5'

    if not osp.isfile(first_chunk):
        return None

    try:
        with h5py.File(first_chunk, 'r') as hf:
            input_files = hf.attrs.get('input_files')

            return None if input_files is None else len(input_files)
    except OSError:
        return None

def auto_chunk_size(n_files:int, ncores:int|None=None, tasks_per_core:int=2,
                    min_chunk_size:int=8, max_chunk_size:int=256)->int:
    """Number of ROOT files to convert within a single task. Chosen such that enough tasks
    exist to keep all cores busy, while avoiding an excessive amount of small HDF5 files.

    Args:
        n_files (int): total number of ROOT files to convert
        ncores (int | None, optional): number of workers. Defaults to cpu_count().
        tasks_per_core (int, optional): tasks to aim for per core. Defaults to 2.
        min_chunk_size (int, optional): lower bound. Defaults to 8.
        max_chunk_size (int, optional): upper bound. Defaults to 256.

    Returns:
        int: files per chunk
    """

    ncores = cpu_count() if ncores is None else ncores

    return int(min(max_chunk_size, max(min_chunk_size, ceil(n_files / max(1, ncores * tasks_per_core)))))

class ROOT2HDF5Converter:
    def __init__(self, root_files:list[str], output_file:str, tree:str, branch:str,
                 output_bname:str|None=None, output_name:str|None=None, dtype:str|None=None,
                 clamp:tuple[int|float|None, int|float|None]|None=None, nan_to:float|int|None=None):
        """_summary_

        Args:
            root_files (list[str]): list of paths to input ROOT files
            output_file (str): output HDF5 file
            tree (str): name of TTree
            branch (str): name of branch in TTree
            output_bname (str | None, optional): basename of HDF5 file. Defaults to None.
            output_name (str | None, optional): name of virtual dataset to create in output_file.
                Will use branch if None. Defaults to None.
            dtype (str | None, optional): Data type. Will be inferred automatically if None. Defaults to None.
            clamp (tuple|None): (min, max) value to clamp to, if not None. Defaults to None.
            nan_to (float|int|None): a value to replace NaN values with, if not None. Defaults to None.
        """
    
        assert(output_file.lower().endswith('.h5') or output_file.lower().endswith('.hdf5'))

        if output_bname is None:
            output_bname = f'{osp.dirname(output_file)}/items/{tree}.{branch.replace("/", ".")}/item'

        if output_name is None:
            output_name = branch.replace("/", ".")
        
        self._root_files = root_files
        self._vds_file = output_file
        
        self._tree = tree
        self._branch = branch
    
        self._output_bname = output_bname
        self._output_name = output_name
        self._dtype = dtype
        self._clamp = (None, None) if clamp is None else (None if clamp[0] is None else clamp[0], None if clamp[0] is None else clamp[1])
        self._nan_to = nan_to

        if not osp.isdir(osp.dirname(output_bname)):
            makedirs(osp.dirname(output_bname), exist_ok=True)
    
    def getChunks(self, **kwargs):
        return create_chunks(self._tree, self._branch, self._root_files, self._output_bname, self._clamp, self._nan_to, **kwargs)

    def getRootFiles(self)->list[str]:
        return self._root_files

    def getOutputBasename(self)->str:
        return self._output_bname

    def getItemSpec(self, chunk_idx:int, overwrite_if_exists:bool=True, keep_dim:bool=False)->GroupedItemSpec:
        """Description of this item as expected by per_chunk_grouped().

        Args:
            chunk_idx (int): index of the chunk to describe
            overwrite_if_exists (bool, optional): Defaults to True.
            keep_dim (bool, optional): Defaults to False.

        Returns:
            GroupedItemSpec: (tree, branch, output_file, overwrite_if_exists, dtype, clamp, nan_to, keep_dim)
        """

        return (self._tree, self._branch, f'{self._output_bname}.{chunk_idx}.h5',
                overwrite_if_exists, self._dtype, self._clamp, self._nan_to, keep_dim)
    
    def checkExisting(self, chunks:list, check_requires_exact_path_match:bool)->tuple[bool, list[int], list[int]]:
        already_done = False
        sizes = []
        ncols_found = 0
        nrows_found = 0
        shape = []

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

                        shape = list(cast(h5py.Dataset, hf['shape'])[:])
                        sizes.append(shape[0])
                        ncols_found = 1 if len(shape) == 1 else shape[1]

                        if i == 0:
                            ncols = ncols_found
                            if self._dtype is None:
                                self._dtype = str(hf.attrs.get('dtype'))
                        else:
                            assert(ncols == ncols_found)

                        input_files:list[str] = hf.attrs['input_files']

                        if check_requires_exact_path_match:
                            path_checks = input_files == chunk_files
                            path_check = np.all(path_checks)
                        else:
                            bnames_found = [osp.basename(osp.normpath(f)) for f in input_files]
                            bnames_expected = [osp.basename(osp.normpath(f)) for f in chunk_files]

                            path_checks = [bnames_found[i] == bnames_expected[i] for i in range(len(bnames_found))]
                            path_check = all(path_checks)

                        already_done = already_done and cast(bool, 
                            hf.attrs['chunk_idx'] == chunk_idx and
                            hf.attrs['tree'] == self._tree and
                            hf.attrs['branch'] == self._branch and
                            path_check)
                        
                        if not already_done:
                            print('chunk_idx [found, expected]:', hf.attrs['chunk_idx'], chunk_idx)
                            print('tree [found, expected]:', hf.attrs['tree'], self._tree)
                            print('branch [found, expected]:', hf.attrs['branch'], self._branch)
                            print('input_files match:', path_check)
                            print('non-matching files: <found>:<expected>')
                            
                            for idx, matches in enumerate(path_checks):
                                if not matches:
                                    print(check_requires_exact_path_match)
                                    print(f'<{input_files[i]}>:<{chunk_files[i]}>' if check_requires_exact_path_match else f'<{bnames_found[i]}>:<{bnames_expected[i]}>')

                            raise Exception(f'File <{out_file}> for chunk <{chunk_idx}> does not fit to expected '+
                                            'data structure. See above print for property=<found> <expected>')
                else:
                    already_done = False
                    raise Exception(f'File <{out_file}> for chunk <{chunk_idx}> does not exist.'+
                                    ' Please delete all chunks to make sure everything is consistent')

                #chunk_idx, tree, branch, root_files[i:i+chunk_size], f'{out_bname}.{chunk_idx}.h5', overwrite_if_exists, read_size, dtype = chunk
                #fpath = f'{self._output_bname}.{chunk_idx}.h5'

            nrows_found = int(np.sum(sizes))
        
        if len(shape):
            tot_shape = [*shape]
            tot_shape[0] = nrows_found
        else:
            tot_shape = [nrows_found, ncols_found]

        return (already_done, tot_shape, sizes)

    def convertLazy(self, nrows:int|None=None, check_existing:bool=False,
                    check_requires_exact_path_match:bool=False, use_vds:bool=False, **kwargs)->tuple[AbstractTask, list[AbstractTask]]:
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
            done, shape, sizes = self.checkExisting(chunks, check_requires_exact_path_match=check_requires_exact_path_match)
            ncols = shape[1] if len(shape) == 2 else 1
        else:
            print(f'No existing (first) chunk found for Tree:Branch <{self._tree}:{self._branch}>. '+
                  f'Proceeding with conversion...')

        h5_files = [chunk[4] for chunk in chunks]
        conversion_tasks = [] if done else [AbstractTask(f'ROOT2HDF5Task:{self._tree}.{self._branch}', per_chunk, (chunk, )) for chunk in chunks]
        
        finalization_task = (CreateVDSTask if use_vds else CombineDatasetsTask)(('CreateVDS' if use_vds else 'CombineDatasets')+f':{self._tree}.{self._branch}',
                                        args=(h5_files, self._vds_file, self._output_name, self._dtype, done, sizes, ncols))
        finalization_task.requires('conversion', conversion_tasks)

        return finalization_task, conversion_tasks

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
            done, shape, sizes = self.checkExisting(chunks, check_requires_exact_path_match=False)
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

            assert(len(first_shape) <= 2)

            ndims = len(first_shape)
            keep_dim = chunks[0][8]
            save_columnwise = ndims == 1 or (ndims >= 1 and not keep_dim) 

            if save_columnwise:
                ncols = first_shape[1] if len(first_shape) == 2 else 1
                shape = (nrows_found,) if len(first_shape) == 1 else (nrows_found, ncols)
            else:
                # use raw shape output
                shape_mod = np.array(first_shape)
                shape_mod[0] = np.sum(sizes)

                ncols = None
                shape = tuple(shape_mod)

            #print(shape)
            #is_1d = 
            #h5_files = sorted(glob(f'/data/dust/user/bliewert/zhh/buffer_test/item.{TREE}.{BRANCH}.*h5'), key=lambda x: int(x.split('.')[-2]))
        
        h5_files = [chunk[4] for chunk in chunks]
        
        createVDS(h5_files, self._vds_file, self._output_name, ncols=ncols, dtype=self._dtype, sizes=sizes)

def createVDS(h5_files:list[str], output_file:str, output_name:str, ncols:int|None=None, dtype:str|None=None, sizes:list[int]|None=None):
    """_summary_

    Args:
        h5_files (list[str]): _description_
        output_file (str): _description_
        output_name (str): _description_. Defaults to None.
        ncols (int|None): if supplied, will assume 2D shape of (nrows, ncols) where nrows is auto-inferred from the HDF5 files
        dtype (str | None, optional): _description_. Defaults to None.
        sizes (list[int] | None, optional): _description_. Defaults to None.
    """
    if dtype is None:
        with h5py.File(h5_files[0], 'r') as hf:
            dtype = hf.attrs.get('dtype')

    if sizes is None:
        sizes = []
        for p in h5_files:
            with h5py.File(p, 'r') as hf:
                sizes.append(cast(h5py.Dataset, hf['shape'])[0])
    
    size = np.sum(sizes)

    # is_multidim = ncols is None and len(shape) > 1 # only true is is_multidim and keep_dim before
    shapes:dict[str, np.ndarray] = {}
    
    with h5py.File(h5_files[0], 'r') as hf:
        column_names = list(cast(Sequence, hf.attrs.get('col_names')))
        #print('column_names', column_names)

        for column in column_names:
            shapes[column] = np.array(cast(h5py.Dataset, hf[column]).shape)
            shapes[column][0] = size

            # avoid shapes (N, 1) for 1-dimensional arrays
            if shapes[column][-1] == 1:
                shapes[column] = shapes[column][:-1]

    with h5py.File(output_file, 'a') as hf:
        for column in column_names:
            shape = shapes[column]
            layout = h5py.VirtualLayout(tuple(shape), dtype)

            counter = 0

            for i, path in enumerate(h5_files):
                ncur = sizes[i]
                cur_shape = np.copy(shape)
                cur_shape[0] = ncur

                # print(output_name, column, tuple(cur_shape))

                vsource = h5py.VirtualSource(path, column, shape=tuple(cur_shape), dtype=dtype)
                layout[counter:(counter+ncur)] = vsource
                counter += ncur

            # Add virtual dataset to output file
            output_column_name = f'{output_name}.{column}' if (ncols is not None and ncols != 1) else output_name
            #print(column, counter, shape, ncols, output_column_name, 'output_column_name in hf.keys()=', output_column_name in hf.keys())
            if output_column_name in hf.keys():
                #print('warning: deleting key', output_column_name)
                del hf[output_column_name]

            hf.create_virtual_dataset(output_column_name, layout, fillvalue=np.nan)
    return True

class CombineDatasetsTask(AbstractTask):
    """Makes data accessible at the main file main_file by copying all converted data from the individual
    sub files. May improve access speed for large datasets, but increases storage consumption

    Args:
        AbstractTask (_type_): _description_
    """

    def work(self, h5_files:list[str], main_file:str, output_name:str, dtype:str|None, done:bool, sizes:list[int], ncols:int, **kwargs):
        conversion:list[ChunkedConversionResult] = kwargs['conversion']

        with h5py.File(h5_files[0], 'r') as hf:
            column_names = list(cast(Sequence, hf.attrs.get('col_names')))
        print('column_names', column_names)

        for column in column_names:
            data = []
            for file in h5_files:
                with h5py.File(file) as hf:
                    data.append(hf['dim0'][:])
            data = np.concatenate(data)
                
            with h5py.File(main_file, 'a') as hf:
                output_column_name = f'{output_name}.{column}' if (ncols is not None and ncols != 1) else output_name

                hf.create_dataset(output_column_name, shape=data.shape, dtype=dtype, chunks=True, fillvalue=np.nan)
                hf[output_column_name][:] = data

        return True

class CreateVDSTask(AbstractTask):
    """Makes data accessible at the main file vds_file by linking to the individual sub-files
    using HDF5's virtual dataset functionality.

    Args:
        AbstractTask (_type_): _description_
    """
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
        return createVDS(h5_files, vds_file, output_name, ncols=ncols, dtype=dtype, sizes=sizes)

class GroupedConversionPlan:
    """Plans the ROOT->HDF5 conversion of multiple items (i.e. TTree branches) which share
    the same list of input ROOT files.

    ROOT2HDF5Converter.convertLazy() schedules one task per item and chunk of ROOT files, so
    every ROOT file is opened and the metadata of its TTrees is parsed once per item. As this
    dominates the runtime, this class instead schedules one task per chunk of ROOT files which
    converts all pending items in a single pass over these files (see per_chunk_grouped).

    The on-disk layout is unchanged, so items converted by earlier versions are still detected
    as done and are re-used. Items which already exist keep their original chunking; the chunk
    size of the pending ones is chosen such that all cores are kept busy (see auto_chunk_size).

    Usage:
        plan = GroupedConversionPlan(converters)
        runner.queueTasks(plan.conversion_tasks); runner.run()
        for i in range(len(converters)):
            plan.finalization_tasks[i].run(conversion=plan.getConversionResults(i))
    """

    def __init__(self, converters:list[ROOT2HDF5Converter], check_existing:bool=True,
                 check_requires_exact_path_match:bool=False, use_vds:bool=True,
                 chunk_size:int|None=None, read_size:int|None=None, ncores:int|None=None,
                 keep_dim:bool=False):
        """
        Args:
            converters (list[ROOT2HDF5Converter]): items to convert. All of them must have been
                created with the same list of ROOT files
            check_existing (bool, optional): whether to skip items which have been converted in
                a previous run. Defaults to True.
            check_requires_exact_path_match (bool, optional): see ROOT2HDF5Converter.checkExisting.
                Defaults to False.
            use_vds (bool, optional): link the converted data using virtual datasets instead of
                copying it. Defaults to True.
            chunk_size (int | None, optional): ROOT files per task. If None, chosen automatically.
                Defaults to None.
            read_size (int | None, optional): ROOT files to hold in memory at a time. If None,
                chosen from the number of items to convert. Defaults to None.
            ncores (int | None, optional): number of workers the tasks will be executed with.
                Defaults to cpu_count().
            keep_dim (bool, optional): keep the shape of >= 2D branches. Defaults to False.
        """

        self._converters = converters
        self._done:list[bool] = []
        self.finalization_tasks:list[AbstractTask] = []
        self.conversion_tasks:list[AbstractTask] = []

        # chunk size => one task per chunk, for each group of items sharing that chunking
        self._group_tasks:dict[int, list[AbstractTask]] = {}

        # (chunk size, index within the group) per item, for extracting its conversion results
        self._item_position:list[tuple[int, int]] = []

        if not len(converters):
            return

        root_files = converters[0].getRootFiles()
        assert(all([conv.getRootFiles() == root_files for conv in converters]))

        default_chunk_size = auto_chunk_size(len(root_files), ncores) if chunk_size is None else chunk_size

        if read_size is None:
            # bounds the memory used per worker: all pending items of read_size files are
            # held in memory at a time
            read_size = max(1, min(16, ceil(96 / max(1, len(converters)))))

        # group items which are not converted yet by their chunk size; items converted in a
        # previous run must keep the chunking they were created with
        groups:dict[int, list[tuple[int, ROOT2HDF5Converter]]] = {}
        chunk_files:dict[int, list[list[str]]] = {}

        for i, conv in enumerate(converters):
            item_chunk_size = detect_existing_chunk_size(conv.getOutputBasename())
            if item_chunk_size is None:
                item_chunk_size = default_chunk_size

            chunks = conv.getChunks(chunk_size=item_chunk_size, read_size=read_size, keep_dim=keep_dim)

            done = False
            ncols = 1
            sizes:list[int] = []

            if check_existing:
                done, shape, sizes = conv.checkExisting(chunks, check_requires_exact_path_match=check_requires_exact_path_match)
                ncols = shape[1] if len(shape) == 2 else 1
            else:
                print(f'No existing (first) chunk found for Tree:Branch <{conv._tree}:{conv._branch}>. '+
                      'Proceeding with conversion...')

            h5_files = [chunk[4] for chunk in chunks]

            finalization_task = (CreateVDSTask if use_vds else CombineDatasetsTask)(
                ('CreateVDS' if use_vds else 'CombineDatasets')+f':{conv._tree}.{conv._branch}',
                args=(h5_files, conv._vds_file, conv._output_name, conv._dtype, done, sizes, ncols))

            self.finalization_tasks.append(finalization_task)
            self._done.append(done)

            if done:
                self._item_position.append((-1, -1))
            else:
                group = groups.setdefault(item_chunk_size, [])
                chunk_files[item_chunk_size] = [chunk[3] for chunk in chunks]

                self._item_position.append((item_chunk_size, len(group)))
                group.append((i, conv))

        # one task per group and chunk of ROOT files, converting all items of the group
        for group_key, group in groups.items():
            tasks:list[AbstractTask] = []

            for chunk_idx, files in enumerate(chunk_files[group_key]):
                item_specs = [conv.getItemSpec(chunk_idx, keep_dim=keep_dim) for _, conv in group]

                tasks.append(AbstractTask(f'ROOT2HDF5Task:chunk {chunk_idx} ({len(item_specs)} items)',
                                          per_chunk_grouped, ((chunk_idx, files, read_size, item_specs), )))

            self._group_tasks[group_key] = tasks
            self.conversion_tasks += tasks

    def isDone(self, item_index:int)->bool:
        """Whether the item had already been converted in a previous run.

        Args:
            item_index (int): index of the item in the converters passed to the constructor

        Returns:
            bool: True if no conversion was scheduled for this item
        """

        return self._done[item_index]

    def getConversionResults(self, item_index:int)->list[ChunkedConversionResult]:
        """Conversion results of one item, in the format expected by CreateVDSTask. Must be
        called after all conversion_tasks have been executed.

        Args:
            item_index (int): index of the item in the converters passed to the constructor

        Returns:
            list[ChunkedConversionResult]: (chunk_idx, (shape, dtype)) per chunk, ordered by chunk
        """

        if self._done[item_index]:
            return []

        group_key, position = self._item_position[item_index]

        results:list[ChunkedConversionResult] = []
        for task in self._group_tasks[group_key]:
            chunk_idx, per_item = cast(GroupedChunkResult, task.getResult())
            results.append((chunk_idx, per_item[position]))

        results.sort(key=lambda x: x[0])

        return results

def tree_n_rows(sources:list[str], tree:str, use_uproot:bool=True, use_mp:bool=True, aggregate:bool=True, return_path:bool=False)->int|tuple[list[str],int]|list[int]:
    """Returns the number of entries in a TTree tree in the files sources
    If aggregate=True, the sum is returned, otherwise a list of sizes in
    equal order as sources will be returned.

    Args:
        sources (list[str]): _description_
        tree (str): _description_
        use_uproot (bool, optional): _description_. Defaults to True.
        use_mp (bool, optional): _description_. Defaults to True.
        aggregate (bool, optional): _description_. Defaults to True.
        return_path (bool, optional): _description_. Defaults to False.

    Returns:
        int|tuple[list[str],int]|list[int]: _description_
    """

    if use_mp and len(sources) > cpu_count():
        # one chunk per core when aggregating, a few more otherwise to balance the load
        chunk_size = ceil(len(sources) / (cpu_count() * (1 if aggregate else 4)))
        chunks = [sources[i:i + chunk_size] for i in range(0, len(sources), chunk_size)]

        outputs:dict[str, int|list[int]] = {}

        with Pool() as pool:
            progress = tqdm(range(len(chunks)))
            progress.set_description(f'Fetching size of TTree <{tree}> in <{len(sources)}> files using <{cpu_count()}> cores and <{len(chunks)}> chunks...')

            for chunk, output in pool.imap_unordered(functools.partial(tree_n_rows, tree=tree, use_uproot=use_uproot,
                                                                       use_mp=False, return_path=True, aggregate=aggregate), chunks):
                outputs[chunk[0]] = output
                progress.update(1)

        if aggregate:
            return int(np.sum([cast(int, output) for output in outputs.values()]))
        else:
            nrows:list[int] = []
            for chunk in chunks:
                nrows += cast(list, outputs[chunk[0]])

            return nrows
    else:
        nrows = 0 if aggregate else []

        if not TYPE_CHECKING:
            if use_uproot:
                for s in sources:
                    with ur.open(s) as uf:
                        if aggregate:
                            nrows += uf[tree].num_entries
                        else:
                            nrows.append(uf[tree].num_entries)
            else:
                import ROOT

                chain = ROOT.TChain(tree)

                for file in sources:
                    chain.Add(file)

                nrows = chain.GetEntries()
        
        return (sources, nrows) if return_path else nrows

def translate_item(sources:list[str], tree:str, names:str|list[str], use_uproot:bool=True)->list['ak.Array']:
    import awkward as ak

    result = []

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

def translate_items(sources:list[str], tree_branches:dict[str, list[str]])->dict[tuple[str, str], list['ak.Array']]:
    """Reads multiple branches, possibly spread over multiple TTrees, from all ROOT files
    in sources. Every file is opened and the metadata of every requested TTree is parsed
    only once, which is what makes this much faster than calling translate_item() per
    branch: parsing the TTree metadata (i.e. constructing the objects for all of its
    branches) dominates the runtime of reading a handful of branches per file.

    Args:
        sources (list[str]): paths to the ROOT files to read
        tree_branches (dict[str, list[str]]): TTree name => list of branches to read from it

    Returns:
        dict[tuple[str, str], list[ak.Array]]: (tree, branch) => one array per entry in sources
    """

    result:dict[tuple[str, str], list['ak.Array']] = {
        (tree, branch): [] for tree, branches in tree_branches.items() for branch in branches }

    for path in sources:
        with ur.open(path) as rf:
            for tree, branches in tree_branches.items():
                ttree = rf[tree]

                for branch in branches:
                    result[(tree, branch)].append(ttree[branch].array())

    return result

def translate_items_lazy(sources:list[str], tree_branches:dict[str, list[str]], n_per_iter:int|None):
    """Batch-wise version of translate_items(); yields the arrays of at most n_per_iter
    ROOT files at a time to keep the memory consumption bounded.

    Args:
        sources (list[str]): paths to the ROOT files to read
        tree_branches (dict[str, list[str]]): TTree name => list of branches to read from it
        n_per_iter (int|None): number of files per batch. If None, all files are read at once

    Yields:
        tuple[list[str], dict[tuple[str, str], list[ak.Array]]]: files of the batch and their data
    """

    n_per_iter = len(sources) if n_per_iter is None else n_per_iter

    for i in range(0, len(sources), n_per_iter):
        batch = sources[i:i+n_per_iter]
        yield batch, translate_items(batch, tree_branches)

class ItemChunkWriter:
    """Writes the data of a single (tree, branch) item belonging to one chunk of ROOT files
    into the HDF5 file of that chunk. The data is handed over batch-wise via write(), so
    that the arrays of several items can be read from the same ROOT files in one pass.

    Supports 1D branches (vectors of primitives), named branches (compound objects such as
    PxPyPzEVectors, saved as one dataset per field) and >= 2D branches (saved either column-
    wise or, with keep_dim=True, keeping their shape).
    """

    def __init__(self, chunk_idx:int, tree:str, branch:str, output_file:str, file_paths:list[str],
                 dtype:str|None=None, clamp:tuple[float|int|None, float|int|None]=(None, None),
                 nan_to:float|int|None=None, keep_dim:bool=False):
        """
        Args:
            chunk_idx (int): index of the chunk this writer belongs to
            tree (str): name of the TTree the data originates from
            branch (str): name of the branch the data originates from
            output_file (str): HDF5 file to create
            file_paths (list[str]): ROOT files of this chunk; saved for the integrity check
            dtype (str | None, optional): if None, inferred from the first batch. Defaults to None.
            clamp (tuple, optional): (min, max) to clamp the values to. Defaults to (None, None).
            nan_to (float | int | None, optional): value to replace NaNs with. Defaults to None.
            keep_dim (bool, optional): keep the shape of >= 2D branches. Defaults to False.
        """

        self._chunk_idx = chunk_idx
        self._tree = tree
        self._branch = branch
        self._dtype = dtype
        self._clamp = clamp
        self._nan_to = nan_to
        self._keep_dim = keep_dim

        self._nrows = 0
        self._n_batches = 0
        self._col_names:list[str] = []
        self._total_shape:list[int] = []
        self._is_named = False
        self._is_1d = False
        self._is_multidim = False

        self._hf = h5py.File(output_file, 'w')
        self._hf.attrs['chunk_idx'] = chunk_idx
        self._hf.attrs['tree'] = tree
        self._hf.attrs['branch'] = branch
        self._hf.attrs['input_files'] = file_paths

    def write(self, result:list['ak.Array'], file_paths:list[str]|None=None):
        """Appends the arrays of one batch of ROOT files to the datasets of this item.

        Args:
            result (list[ak.Array]): one array per ROOT file of the batch
            file_paths (list[str] | None, optional): files the arrays originate from; only
                used to give a helpful error message. Defaults to None.
        """

        import awkward as ak

        hf = cast(h5py.File, self._hf)
        keep_dim = self._keep_dim

        ndims = result[0].ndim
        columns = result[0].fields

        # the case if tree/branch is a compound object like a PxPyPzEVector
        is_named = self._is_named = len(columns) > 0

        # the case if tree/branch refers to a vector of primitives/scalars (float, int)
        is_1d = self._is_1d = ndims == 1

        # the case if tree/branch refers to a >= 2 dim. object (matrix, tensor).
        # first dimension will be interpreted as batch dimension
        is_multidim = self._is_multidim = ndims > 1

        assert(is_named or is_1d or is_multidim)

        # regularize
        if is_multidim:
            for i in range(len(result)):
                try:
                    result[i] = ak.to_regular(result[i])
                except Exception as e:
                    origin = f'file {file_paths[i]}: with' if file_paths is not None else 'item'
                    print(f'Error converting {origin} tree={self._tree} branch={self._branch}'+
                          f' columns={", ".join(columns)}')
                    raise e

        size = sum([len(result[i]) for i in range(len(result))])

        if self._n_batches == 0:
            # infer ncols+dtype from the first entry
            if is_named:
                self._col_names = columns
            elif is_1d:
                self._col_names = ['dim0']
            elif is_multidim:
                self._col_names = ['dim0'] if keep_dim else [f'dim{col}' for col in range(result[0].type.content.size)]

            if is_multidim and keep_dim:
                # get type of multidim object
                first_as_np = np.array(result[0])
                self._total_shape = list(first_as_np.shape)
                self._total_shape[0] = size
                self._dtype = str(first_as_np.dtype)

            if self._dtype is None:
                if is_named:
                    # get type of primitivies saved in compound object
                    self._dtype = str(result[0].type.content.content(result[0].fields[0]))
                else:
                    # get type of 1d and column-wise 2d obejcts
                    self._dtype = str(result[0].type.content) if is_1d else str(result[0].type.content.content)

            for col in self._col_names:
                # chunk-size 16MB
                # using size for per-column storage
                # using total_shape for keep_dim in case of multidimensional object
                ds_shape = size
                ds_maxshape = (None, )

                if is_multidim and keep_dim:
                    ds_shape = tuple(self._total_shape)
                    ds_maxshape = [*self._total_shape]
                    ds_maxshape[0] = None

                hf.create_dataset(col, shape=ds_shape, maxshape=ds_maxshape, dtype=self._dtype,
                                  chunks=True, rdcc_nbytes=16*1024**2, fillvalue=np.nan)

        for i_col, col in enumerate(self._col_names):
            if self._n_batches and not (is_multidim and keep_dim):
                cast(h5py.Dataset, hf[col]).resize((self._nrows+size, ))

            counter = 0
            for arr in result:
                arr_size = len(arr)
                dataset:h5py.Dataset = cast(h5py.Dataset, hf[col])

                if is_named:
                    target_data = arr[col][:]
                else:
                    if is_multidim and not keep_dim:
                        target_data = arr[:, i_col]
                    else:
                        target_data = arr

                if self._clamp[0] is not None or self._clamp[1] is not None:
                    target_data = np.clip(target_data, a_min=self._clamp[0], a_max=self._clamp[1], dtype=self._dtype)

                if self._nan_to is not None:
                    if not isinstance(target_data, np.ndarray):
                        target_data = np.array(target_data)

                    target_data[np.isnan(target_data)] = self._nan_to

                dataset[(self._nrows+counter):(self._nrows+counter+arr_size)] = target_data
                counter += arr_size

            assert(counter == size)

        self._nrows += size
        self._n_batches += 1

    def close(self)->tuple[tuple, str]:
        """Writes the shape and metadata of this item and closes the HDF5 file.

        Returns:
            tuple[tuple, str]: total shape and dtype of the converted data
        """

        hf = cast(h5py.File, self._hf)

        total_shape = self._total_shape

        if len(total_shape) == 0:
            # note: the column count is intentionally left at 0 here (as it always was);
            # the code reading this back only evaluates len(shape) and shape[0], while
            # createVDS() derives the column layout from the col_names attribute. changing
            # it would rename the datasets createVDS() creates in the main HDF5 file
            total_shape = (self._nrows, ) if self._is_1d else (self._nrows, 0)

        hf['shape'] = np.array(total_shape, dtype=int)
        hf.attrs['col_names'] = self._col_names
        hf.attrs['save_columnwise'] = self._is_1d or self._is_named or not (self._is_multidim and self._keep_dim)
        hf.attrs['dtype'] = self._dtype

        hf.close()
        self._hf = None

        assert(isinstance(self._dtype, str))

        return (total_shape if isinstance(total_shape, tuple) else tuple(total_shape), self._dtype)

    def abort(self):
        """Closes the HDF5 file without finalizing it. Does nothing after close()."""

        if self._hf is not None:
            self._hf.close()
            self._hf = None

def read_existing_chunk(output_file:str, dtype:str|None)->tuple[tuple, str]:
    """Reads shape and dtype of a chunk file that has been converted previously.

    Args:
        output_file (str): HDF5 file of the chunk
        dtype (str | None): expected dtype; checked against the stored one if not None

    Raises:
        Exception: if the stored dtype does not match the expected one

    Returns:
        tuple[tuple, str]: shape and dtype of the converted data
    """

    with h5py.File(output_file, 'r') as hf:
        shape = tuple(cast(h5py.Dataset, hf['shape'])[:])
        dtype_read = str(hf.attrs.get('dtype'))

    if dtype is not None and dtype != dtype_read:
        raise Exception(f'dtype mismatch: expected <{dtype}> but found <{dtype_read}>')

    return (shape, dtype_read)

def per_chunk(args:tuple[int, str, str, list[str], str, bool, int|None,
                         str|None, tuple[float|int|None, float|int|None], float|int|None, bool])->ChunkedConversionResult:
    """Attempts to read tree/branch from all ROOT files in file_paths and writes the result
    to the HDF5 file outp under one dataset per column (see ItemChunkWriter).
    Values must be of regular shape. Supports 1D and 2D TTree branches.

    Consider per_chunk_grouped() when more than one branch should be converted: it reads all
    of them in a single pass over the ROOT files.

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
        args[8] = clamp (tuple[float|int|None, float|int|None])
        args[9] = nan_to (float|int|None)
        args[10] = keep_dim (bool)

    Returns:
        ChunkedConversionResult: (chunk_idx, (total_shape, dtype))
    """

    chunk_idx:int = args[0]
    tree:str = args[1]
    branch:str = args[2]
    file_paths:list[str] = args[3]
    output_file:str = args[4]
    overwrite_if_exists:bool = args[5]
    read_size:int|None = args[6]
    dtype:str|None = args[7]
    clamp:tuple[float|int|None, float|int|None] = args[8]
    nan_to:float|int|None = args[9]
    keep_dim:bool = bool(args[10])

    result = per_chunk_grouped((chunk_idx, file_paths, read_size,
                                [(tree, branch, output_file, overwrite_if_exists, dtype, clamp, nan_to, keep_dim)]))

    return (chunk_idx, result[1][0])

def per_chunk_grouped(args:tuple[int, list[str], int|None, list[GroupedItemSpec]])->GroupedChunkResult:
    """Converts multiple items (i.e. TTree branches) of one chunk of ROOT files at once.
    Every ROOT file is opened once and the metadata of every involved TTree is parsed once,
    no matter how many branches are requested from it. As this is what dominates the runtime
    of the conversion, this is much faster than calling per_chunk() per item.

    Each item is written to its own HDF5 file, exactly as per_chunk() does, so the output is
    compatible with data converted by previous versions.

    Args:
        args[0] = chunk_idx (int): index of the chunk of ROOT files
        args[1] = file_paths (list[str]): ROOT files of this chunk
        args[2] = read_size (int|None): how many files should be loaded into memory at a
            time. If None, all files of the chunk are read at once
        args[3] = items (list[GroupedItemSpec]): items to convert, given as
            (tree, branch, output_file, overwrite_if_exists, dtype, clamp, nan_to, keep_dim)

    Returns:
        GroupedChunkResult: (chunk_idx, { index of the item in args[3]: (total_shape, dtype) })
    """

    chunk_idx:int = args[0]
    file_paths:list[str] = args[1]
    read_size:int|None = args[2]
    items:list[GroupedItemSpec] = args[3]

    results:dict[int, tuple[tuple, str]] = {}
    pending:list[tuple[int, GroupedItemSpec]] = []

    # load already converted items from HDF5
    for i, item in enumerate(items):
        output_file, overwrite_if_exists, dtype = item[2], item[3], item[4]

        if osp.isfile(output_file) and not overwrite_if_exists:
            results[i] = read_existing_chunk(output_file, dtype)
        else:
            pending.append((i, item))

    if len(pending):
        # several items may write to the same file, e.g. when the same branch is exposed under
        # two names; these are converted once and share the result
        items_of_output:dict[str, list[int]] = {}
        spec_of_output:dict[str, GroupedItemSpec] = {}

        for i, item in pending:
            output_file = item[2]

            if output_file in spec_of_output:
                if spec_of_output[output_file][4:] != item[4:]:
                    raise Exception(f'Items <{spec_of_output[output_file][0]}:{spec_of_output[output_file][1]}> and '+
                                    f'<{item[0]}:{item[1]}> are both converted to <{output_file}> but request a '+
                                    'different dtype, clamp, nan_to or keep_dim')
            else:
                spec_of_output[output_file] = item

            items_of_output.setdefault(output_file, []).append(i)

        # collect the branches to read per TTree
        tree_branches:dict[str, list[str]] = {}
        for tree, branch, *_rest in spec_of_output.values():
            branches = tree_branches.setdefault(tree, [])
            if branch not in branches:
                branches.append(branch)

        writers:dict[str, ItemChunkWriter] = {}

        try:
            for output_file, (tree, branch, _, _, dtype, clamp, nan_to, keep_dim) in spec_of_output.items():
                writers[output_file] = ItemChunkWriter(chunk_idx, tree, branch, output_file, file_paths,
                                                       dtype=dtype, clamp=clamp, nan_to=nan_to, keep_dim=keep_dim)

            for batch_files, batch in translate_items_lazy(file_paths, tree_branches, read_size):
                for output_file, (tree, branch, *_rest) in spec_of_output.items():
                    # copy the list as ItemChunkWriter.write() may regularize its entries
                    writers[output_file].write(list(batch[(tree, branch)]), file_paths=batch_files)

            for output_file, writer in writers.items():
                result = writer.close()

                for i in items_of_output[output_file]:
                    results[i] = result
        finally:
            for writer in writers.values():
                writer.abort()

    return (chunk_idx, results)

def process_chunks(chunks, n_files:int|None=None, ncores:int|None=None)->list[ChunkedConversionResult]:
    chunk_outputs = []

    with Pool(ncores if ncores is not None else round(cpu_count() * .8)) as pool:
        progress = tqdm(range(n_files if n_files is not None else len(chunks)))
        
        for chunk_output in pool.imap_unordered(per_chunk, chunks):
            chunk_idx, chunk = chunk_output
            
            progress.update(len(chunks[chunk_idx][3]) if n_files is not None else 1)
            progress.set_description(f'Receiving data for chunk {chunk_idx}')
            
            chunk_outputs.append(chunk_output)
                    
            #self.save()
            
    chunk_outputs.sort(key=lambda x: x[0]) # Sort by file location

    return chunk_outputs