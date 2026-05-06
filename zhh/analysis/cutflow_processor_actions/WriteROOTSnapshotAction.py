from ..CutflowProcessorAction import FileBasedProcessorAction, CutflowProcessor
from zhh.analysis.DataStore import NamedHDF5Mapping, DataStore
from zhh.analysis.DataSource import DataSource

class WriteROOTSnapshotAction(FileBasedProcessorAction):
    def __init__(self, cp: CutflowProcessor, steer: dict, file:str, tree:str='cutflow',
                 include_custom_columns:list[str]=[], include_input_columns:bool=False, include_mva_outputs:bool=False, CHUNK_SIZE:int=65536, **kwargs):
        """Writes out MVA data for training and testing. Assumes a splitting into different categories
        has been performed previously using SplitDatasetsAction.

        Args:
            cp (CutflowProcessor): _description_
            steer (dict): _description_
            file (str): name of output ROOT file
            tree (str): name of the output ROOT TTree
            include_custom_columns (list[str]): list of custom columns to include. Defaults to [].
            include_input_columns (bool): whether or not to include all input feature columns. Defaults to False.
            include_mva_outputs (bool): whether or not to include all MVA output columns. Defaults to False.
            CHUNK_SIZE (int): controls how many entries per batch and branch should be fetched and written at once
        """
        super().__init__(cp, steer)

        assert(file.lower().endswith('.root'))

        self._file = file
        
        var_list = find_vars_from_steer(steer, include_mva_outputs, include_input_columns)
        var_list += include_custom_columns

        self._var_list = var_list
        self._tree = tree
        self._chunk_size = CHUNK_SIZE
        
    def run(self):
        print(f'Writing ROOT snapshot to {self._file}')
        print(f'Properties to be written: {self._var_list}')
        rootSnapshot(self._cp, self._file, self._var_list, self._tree, self._chunk_size)

    def output(self):
        return self.localTarget(self._file)

def parse_columns(sources:list[DataSource], columns:list[str]):
    import h5py

    sizes:list[int] = []
    dtypes:dict[str, str] = { 'source': 'uint8' }
    mappings:dict[str, str] = { }
    
    for i, source in enumerate(sources):
        source.getStore().resetView()
        sizes.append(len(source))

    example_store = sources[0].getStore()
    example_file = example_store._h5_file
    
    with h5py.File(example_file, 'r') as hf:
        keys = list(hf.keys())
        for item in columns:
            if item in keys:
                dtypes[item] = str(hf[item].dtype)
                mappings[item] = item
            elif example_store.hasProperty(item):
                # attempt to fetch mapping, require NamedHDF5Mapping
                mapping = example_store._props[item]
                if not isinstance(mapping, NamedHDF5Mapping):
                    print(f'Warning: Skipping feature {item} in WriteROOTSnapshotAction because {mapping} cannot be used to infer a mapping')
                    continue
                else:
                    dtypes[item] = str(hf[mapping._item].dtype)
                    mappings[item] = mapping._item 
            else:
                print(f'Warning: Skipping feature {item} in WriteROOTSnapshotAction: No data found in HDF5 file {example_file}')
    
    return (sizes, dtypes, mappings)

def find_vars_from_steer(steer,
                         include_mva_outputs:bool,
                         include_input_columns:bool,
                         include_action_columns:bool=False):
    var_list = []
    
    if include_mva_outputs:
        for mva in steer['mvas']:
            var_list += [mva['label_name']]

            for item in mva['classes']:
                category = item[1]

                var_list += [f'{mva["label_name"]}#{category}']

    if include_input_columns:
        for item in steer['features']['interpret']:
            var_list += item['names'] if 'names' in item else [item['name']]
        
        # include final state counts
        from zhh.analysis.PreselectionAnalysis import fs_columns
        var_list += fs_columns

    if include_action_columns:
        for item in steer['actions']:
            for prop in item:
                if 'column' in prop and not item[prop] in var_list:
                    var_list += [item[prop]]
    
    return var_list

def find_vars_from_store(store):
    var_list = store.keys()

    # remove multidim values
    var_list = list(filter(lambda x: not '.dim' in x, var_list))
    var_list.sort()
    
    return var_list

def rootSnapshot(cp, output_file:str, columns:list[str], tree:str, CHUNK_SIZE:int=65536):
    import os.path as osp
    import h5py
    import numpy as np
    import uproot as ur
    from tqdm.auto import tqdm

    sources = cp.getSources()
    sizes, dtypes, mappings = parse_columns(sources, columns)

    if False:
        # ROOT does not support adding branches to an existing tree, so this is of no use
        # instead: add branches to new trees, completely delete/overwrite old trees
        file_exists = osp.isfile(output_file)

        with getattr(ur, 'open' if file_exists else 'create')(output_file) as rf:
            if not file_exists:
                rf.mktree(tree, dtypes)
                missing_keys = list(dtypes.keys())
            else:
                tree = rf[tree]
                print(set(dtypes.keys()))
                print(set(rf[tree].keys()))
                missing_keys = list(set(dtypes.keys()) - set(rf[tree].keys()))
    else:
        missing_keys = list(dtypes.keys())

    with ur.recreate(output_file) as rf:
        #tree = rf['cutflow']
        rf.mktree(tree, dtypes)

        for i, source in enumerate(sources):
            soure_name = source.getName()
            store = source.getStore()
            remaining = sizes[i]
            n_batches = 1 + remaining // CHUNK_SIZE
            k = 0

            with h5py.File(store._h5_file, 'r') as hf:
                datasets = {}
                for column in missing_keys:
                    if column != 'source':
                        datasets[column] = hf[mappings[column]]

                for j in (pbar := tqdm(range(0, sizes[i], CHUNK_SIZE))):
                    this_chunk = min(remaining, CHUNK_SIZE)
                    
                    batch = { }
                    pbar.set_description(f'Writing batch with data from {soure_name}') 
                    #t0 = datetime.now()
                
                    for column in missing_keys:
                        batch[column] = i*np.ones(this_chunk, dtype=np.uint8) if column == 'source' else (
                            datasets[column][j:j+this_chunk] # np.zeros(branch_size, dtype=dtypes[column])
                        )

                    remaining -= CHUNK_SIZE
                    k += 1

                    #print(f'tread={datetime.now() - t0}')

                    #t0 = datetime.now()
                    rf[tree].extend(batch)
                    #print(f'twrite={datetime.now() - t0}')
