from collections.abc import Callable
from .PreselectionAnalysis import sample_weight, get_pol_key
from .DataStore import DataStore
from zhh.processes import parse_polarization_code, ProcessCategories
from zhh.analysis.TTreeInterface import FinalStateCounts
from typing import cast, TYPE_CHECKING
import uproot as ur, h5py, os.path as osp, os, numpy as np

class DataSource:
    def __init__(self, file:str, name:str, work_root:str|None=None):
        """Helper class to calculate weights and prepare data for MVA.

        NEW: if fname ends with .h5, we assume we can read columnar feature data from
        it straight away (see the ROOT2HDF5Converter class). This is the recommended
        workflow. The default argument will remain for backwards compatability.

        Args:
            work_root (str): directory to store Merged.root. must be user writable
            name (str, optional): _description_.
        """

        self._is_ready = False
        
        self._name = name
        self._h5_file = file
        self._npz_file = f'{osp.splitext(file)[0]}.npz'
        self._work_root = work_root if work_root is not None else osp.dirname(file)
        
        # fetchData()
        self._store:DataStore|None = None
        self._size = -1
        
        # weight()
        self._processes:np.ndarray|None = None
        self._lumi_inv_ab:float = 0.
        
        # registerEventCategory(): event selection masks
        self._event_categories:dict[str, tuple[np.ndarray, int|None]] = {}
        self._event_category_resolvers:dict[str, tuple[Callable, int|None]] = {}
        self._evalutatedEventCategories = False
    
    def __repr__(self)->str:
        return f'DataSource<name={self._name}>'
    
    def __len__(self):
        return len(self.getStore())
    
    def getName(self)->str:
        return self._name
    
    def initialize(self, lumi_inv_ab:float, evt_cat_default:int|None, evt_cat_order:list|None,
                    reset:bool=False):
        """Convenience function which fully initializes the DataSource. Either
        loads weights from cache or calculates them and makes them available there,
        if use_cache is True. Also evaluates the event_category column for each
        event using the categories that have been registered previously using
        registerEventCategory, and using the default and order parameter too.

        Args:
            lumi_inv_ab (float): integrated luminosity
            evt_cat_default (int | None): if None, no default category is applied
            evt_cat_order (list | None): if None, the order in which they were re-
                                        gistered is used for evaluation. if [], no
                                        category is registered after the default one
            reset (bool, optional): whether or not to delete all existing cache files.
                                    Defaults to False.
        """

        from os import unlink

        if reset or not osp.isfile(self._npz_file):
            if reset and osp.isfile(self._npz_file):
                unlink(self._npz_file)

            self._store = DataStore(self._h5_file, final_states=True)
                
            weight_data, processes = self.weight(lumi_inv_ab=lumi_inv_ab)
            
            # parses the final state counts, find the first matching category,
            # or set to default_category
            # if category_order is [], the default category is applied to all
            # events
            self.evaluateEventCategories(default_category=evt_cat_default, order=evt_cat_order)
            
            # store the HDF5 file
            store = self.getStore()
            store.itemsSnapshot()

            # store processes in NPZ file
            self.setAttribute('lumi_inv_ab', lumi_inv_ab)
            np.savez_compressed(self._npz_file, processes=processes, weight_data=weight_data, lumi_inv_ab=lumi_inv_ab)
        else:
            print(f'  Reading from <{self._h5_file}> and <{self._npz_file}>')

            # fetch from HDF5 and NPZ
            npz = np.load(self._npz_file)            
            self._processes = npz['processes']
            self._lumi_inv_ab = npz['lumi_inv_ab']
            self._store = DataStore(self._h5_file, final_states=True,
                                    in_memory_writable=['pid', 'pol_code', 'weight', 'event_category'])
    
    def getStore(self)->DataStore:
        """Returns the data store.

        Returns:
            TTreeInterface: columnar interface to all event data
        """
        
        assert(self._store is not None)
        
        return self._store
    
    def plotFinalStateCounts(self):
        from zhh import plot_final_state_counts 
        return plot_final_state_counts(self.getStore())

    def weight(self, lumi_inv_ab:float=2.)->tuple[np.ndarray,np.ndarray]:
        """Extracts cross-sections and number of generated events for each
        polarization-process combination. Then calculates weights for each
        combination (processes, n x M) and event (weight_data, l x K). The
        results are given as two named numpy arrays. processes is stored in
        self._processes. Requires combine() and fetch(). Attached pid and
        weight columns to the preselection summary.
        
        n: number of process-polarization ("proc_pol") combinations
        M: 9 features, e.g. "cross_sec", "proc_pol", "n_events", "weight"
        
        l: number of events in Merged.root
        K: 6 features, e.g. "weight", "process", "polarization_code"

        Args:
            lumi_inv_ab (float, optional): _description_. Defaults to 2..

        Returns:
            tuple[np.ndarray,np.ndarray]: weight_data, processes
        """
        
        self._lumi_inv_ab = lumi_inv_ab
        # fetch data for weight calculation
        store = self.getStore()
        process = store['process']

        weight_data = np.zeros(len(self), dtype=[
            ('pid', 'H'),
            ('process', 'I'),
            ('polarization_code', 'B'),
            ('cross_section', 'f'),
            ('weight', 'f')])
        
        weight_data['process'] = process
        weight_data['polarization_code'] = store['pol_code']
        weight_data['cross_section'] = store['cross_section']

        # get number of processes and polarization combinations
        # and build proc-pol index
        n_combinations = 0
        unq_processes = np.unique(weight_data['process'], return_counts=True)
        for proc in unq_processes[0]:
            n_combinations += np.unique(weight_data[weight_data['process'] == proc]['polarization_code']).size    

        processes = np.zeros(n_combinations, dtype=[
            ('pid', 'H'), # id as in ProcessCategories
            ('process', '<U60'), # string name of process
            ('proc_pol', '<U64'),
            ('pol_e', 'i'),
            ('pol_p', 'i'),
            ('polarization_code', 'B'),
            ('cross_sec', 'f'),
            ('n_events', 'I'),
            ('weight', 'f')])

        idx = 0
        for proc in unq_processes[0]:
            proc = int(proc)
            for polarization_code in np.unique(weight_data[weight_data['process'] == proc]['polarization_code']):
                mask = (weight_data['process'] == proc) & (weight_data['polarization_code'] == polarization_code)
                
                process_name = ProcessCategories.inverted[proc]
                Pem, Pep = parse_polarization_code(polarization_code)

                cross_sec = weight_data[mask]['cross_section'][0]
                n_gen = np.sum(mask)
                wt = sample_weight(cross_sec, (Pem, Pep), n_gen, lumi_inv_ab)
                
                print(f'Process {process_name:12} with Pol e{"L" if Pem == -1 else "R"}.p{"L" if Pep == -1 else "R"}'+
                      f' has {n_gen:9} events xsec={cross_sec:.3E} wt={wt:.3E}')
                procpol = f'{process_name}_{get_pol_key(Pem, Pep)}'
                
                processes[idx]['process'] = process_name
                processes[idx]['proc_pol'] = procpol
                processes[idx]['pol_e'] = Pem
                processes[idx]['pol_p'] = Pep
                processes[idx]['polarization_code'] = polarization_code
                processes[idx]['cross_sec'] = cross_sec
                processes[idx]['n_events'] = n_gen
                processes[idx]['weight'] = wt
                processes[idx]['pid'] = proc
                
                weight_data['weight'][mask] = wt
                weight_data['pid'][mask] = proc
                
                idx += 1
                
        self.getStore()['pid'] = weight_data['pid']
        self.getStore()['weight'] = weight_data['weight']
        self._processes = processes
                
        return weight_data, processes
    
    def getAttribute(self, name:str, if_not_exists=None):
        with h5py.File(self._h5_file) as hf:
            return hf.attrs[name] if name in hf.attrs else if_not_exists                 
        
    def setAttribute(self, name:str, value):
        with h5py.File(self._h5_file) as hf:
            hf.attrs[name] = value
    
    def getIntegratedLuminosity(self)->float:
        """Returns the luminosity in ab^-1.

        Returns:
            float: luminosity
        """
        
        assert(self._lumi_inv_ab > 0.)
        
        return self._lumi_inv_ab
    
    def getProcesses(self)->np.ndarray:
        """Returns the processes array.

        Returns:
            np.ndarray: processes
        """
        
        assert(self._processes is not None)
        
        return self._processes

    def printProcesses(self):
        for proc in self.getProcesses():
            process_name = proc['process']
            Pem = proc['pol_e']
            Pep = proc['pol_p']
            n_gen = proc['n_events']
            cross_sec = proc['cross_sec']
            wt = proc['weight']

            print(f'Process {process_name:12} with Pol e{"L" if Pem == -1 else "R"}.p{"L" if Pep == -1 else "R"} has'+
                  f' {n_gen:9} events xsec={cross_sec:.3E} wt={wt:.3E}')
    
    def getFinalStateCounts(self)->FinalStateCounts:
        """Returns the final state counts array.

        Returns:
            np.ndarray: final state counts
        """
        
        from .DataStore import parse_final_state_counts
        
        assert(self._store is not None)
        
        return parse_final_state_counts(self.getStore())
    
    def registerEventCategory(self, name:str, mask_or_func:Callable|np.ndarray, category:int|None,
                              overwrite:bool=False):
        
        if (name in self._event_categories or name in self._event_category_resolvers) and not overwrite:
            print(f'Event category {name} already registered. Doing nothing.')
        else:
            if name in self._event_categories:
                del self._event_categories[name]
            
            if isinstance(mask_or_func, np.ndarray):
                self._event_categories[name] = (mask_or_func, category)
            elif callable(mask_or_func):
                self._event_category_resolvers[name] = (mask_or_func, category)
        
        return self
    
    def evaluateEventCategories(self, default_category:int|None, force:bool=False, order:list[str]|None=None,
                                reset=False)->'DataSource':
        """Evaluates event category definitions and saves them as numpy masks.
        default_category is assigned to all events first, if not None.
        After that, all categories for which the category paraemter in the
        registerEventCategory() call was not None, are assigned. If order is
        an empty array, nothing is changed after the default category is assigned.  

        Args:
            default_category (int | None): _description_
            force (bool, optional): _description_. Defaults to False.
            order (list[str]|None, optional): if None, all categories are considered
                in the order they were registered. If a list, ONLY the contained categories
                are considered. Defaults to None.

        Returns:
            DataSource: _description_
        """
        from .DataStore import parse_final_state_counts
        
        if not self._evalutatedEventCategories or force:
            print(f'Evaluating event categories for {self._name}...')
            
            store = self.getStore()
            store.resetView()
            
            fsc = parse_final_state_counts(store)

            if not 'event_category' in store.keys():
                store['event_category'] = np.zeros(len(store), dtype='uint8')

            for name, (mask_func, category) in self._event_category_resolvers.items():
                mask = mask_func(self, fsc)
                self._event_categories[name] = (mask, category)
            
            if order is None:
                order = list(self._event_categories.keys())
            
            if default_category is not None:
                store['event_category'][:] = default_category
                print(f' Assigned default event category (ID={default_category}) to all' +
                      f'{len(store["event_category"]):d} events')
            else:
                print(f' No default event category assigned')
            
            mask_sum = np.zeros(len(self._event_categories[next(iter(self._event_categories))][0]), dtype='B')
            
            for i, category_name in enumerate(order):
                mask, replace = self._event_categories[category_name]
                
                print(f' Assigned event category {category_name} (ID={replace}) to {np.sum(mask):d} events with'+
                      f' weight {np.sum(store["weight"][mask]):3g}')
                if replace is not None:
                    store['event_category'][mask] = replace
                
                collision = np.logical_and(mask_sum > 0, mask)
                n_collisions = np.sum(collision)
                if n_collisions > 0:
                    print(f'  Warning: Event category {category_name} is not orthogonal to previous selection.'+
                          f' Found {n_collisions} collisions')
                
                mask_sum += mask
            
            n_assigned = np.sum(mask_sum > 0)
            
            print(f' Total:{ " (after default category)" if default_category is None else "" } Event categories'+
                  f' assigned to {n_assigned:d} events ({(n_assigned / len(mask_sum)):.2%})')
                
            self._evalutatedEventCategories = True
        
        return self
    
    def getCategoryMask(self, name:str)->np.ndarray:
        """Returns a binary mask specifying whether or not an event belongs to the
        event category given by name. 

        Args:
            name (str): event category; must be registered using registerEventCategory
                        before usage 

        Raises:
            ValueError: _description_

        Returns:
            _type_: _description_
        """

        if name not in self._event_categories:
            if name in self._event_category_resolvers:
                from .DataStore import parse_final_state_counts
                
                mask_func, category = self._event_category_resolvers[name]
                mask = mask_func(self, parse_final_state_counts(self.getStore()))
                self._event_categories[name] = (mask, category)
            else:
                raise ValueError(f'Event category {name} not registered.')
        
        return self._event_categories[name][0]
    
    def containsProcess(self, process:str):
        """Checks if the process is in the processes array.

        Args:
            process (str): process name

        Returns:
            bool: True if process is in processes array
        """
        
        assert(self._processes is not None)
        return process in self._processes['process']
    
    def containsFinalState(self, name:str):
        """Checks if the final state is in the processes array.

        Args:
            name (str): final state name

        Returns:
            bool: True if final state is in processes array
        """
        
        assert(self._processes is not None)
        return (name in self._event_categories or name in self._event_category_resolvers)