from collections.abc import Callable, Sequence
from .PreselectionAnalysis import fetch_preselection_data, sample_weight, get_pol_key
from .TTreeInterface import TTreeInterface
from zhh.processes import parse_polarization_code, ProcessCategories
from zhh.analysis.TTreeInterface import FinalStateCounts

import uproot as ur
import os.path as osp
import os, subprocess, numpy as np
from tqdm.auto import tqdm
from typing import cast, Optional, Literal

config = {
    'N_CORES': 8
}

class AnalysisChannel:
    def __init__(self, work_root:str, name:str=''):
        """Helper class to combine ROOT files+TTrees, calculate weights
        and prepare data for TMVA.

        Args:
            work_root (str): directory to store Merged.root. must be user writable
            name (str, optional): _description_. Defaults to ''.
        """
        
        import ROOT
        if config['N_CORES'] is not None:
            ROOT.EnableImplicitMT(config['N_CORES'])
        
        self._work_root = work_root
        
        self._name = name
        self._root_files = []
        self._merged_file = f'{work_root}/Merged.root'
        
        # combine()
        self._rf:ur.WritableFile|None = None
        self._size:int|None = None
        
        # fetchData()
        self._store:TTreeInterface|None = None
        
        # weight()
        self._processes:np.ndarray|None = None
        self._lumi_inv_ab:float = 0.
        
        # registerEventCategory(): event selection masks
        self._event_categories:dict[str, tuple[np.ndarray, int|None]] = {}
        self._event_category_resolvers:dict[str, tuple[Callable, int|None]] = {}
        self._evalutatedEventCategories = False
    
    def __repr__(self)->str:
        return f'AnalysisChannel<name={self._name}>'
    
    def __len__(self):
        if self._size is None:
            raise Exception('.combine() must be called before accessing data')
        
        return self._size        
    
    def getName(self)->str:
        return self._name
        
    def combine(self, trees:list[str]|None=None, root_files:list[str]|None=None)->'AnalysisChannel':
        """Combines all ROOT TTrees in root_files into one "Merged" TTree in Merged.root
        inside work_root using TChain with RDataFrame and its snapshot method. If there
        are branches with the same name in different trees, the first is saved under the
        usual name, and all subsequent ones with their original tree name as prefix.
        Only if Merged.root already exists, trees and root_files will be optional.

        Args:
            trees (list[str] | None): name of all ROOT TTree objects to merge.
                defaults to None.
            root_files (list[str] | None): paths of all ROOT files to merge.
                defaults to None.
        """
        
        import ROOT

        self._root_files = root_files
        
        if not os.path.exists(self._work_root):
            os.makedirs(self._work_root)
        
        if not os.path.isfile(self._merged_file):
            assert(isinstance(root_files, list) and isinstance(trees, list))
            
            ROOT.gInterpreter.GenerateDictionary("ROOT::VecOps::RVec<vector<double>>", "vector;ROOT/RVec.hxx")
            ROOT.gInterpreter.GenerateDictionary("ROOT::VecOps::RVec<ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double>>>", "vector;ROOT/RVec.hxx;Math/Vector4D.h")
            
            chain = ROOT.TChain(trees[0])
            friends = []
            for t in trees[1:]:
                friend = ROOT.TChain(t)
                friends.append(friend)

            for file in root_files:
                for friend in friends:
                    friend.Add(file)

                chain.Add(file)
                
            for friend in friends:
                chain.AddFriend(friend)
            
            #c.Add("/home/ilc/bliewert/jobresults/550-2l4q-ana/E550-TDR_ws.P6f_eexxxx.Gwhizard-3_1_5.eL.pL.I410026.1-0_AIDA.root")
            df = ROOT.RDataFrame(chain)
            df.Snapshot('Merged', self._merged_file)
            
        if self._rf is None:
            self._rf = cast(ur.WritableFile, ur.open(self._merged_file))
            
        self._size = self._rf['Merged'].num_entries
        
        return self

    def getTTree(self)->ur.TTree:
        """Returns the TTree object of the Merged.root file.

        Returns:
            ur.TTree: TTree object of the Merged.root file.
        """
        
        assert(self._rf is not None)
        
        return cast(ur.TTree, self._rf['Merged'])
    
    def fetchData(self, snapshot:str|None=None)->TTreeInterface:
        """Gives lazily loaded access to the tree data and other custom defined
        properties. The pid and weight columns are only populated after weight
        is called. 

        Args:
            presel (Literal['ll', 'vv', 'qq']): which preselection to use

        Returns:
            TTreeInterface: named np array-like object with
                channel specific data.
        """
        assert(self._rf is not None)
        
        if self._store is None:
            self._store = TTreeInterface(cast(ur.TTree, self._rf['Merged']), snapshot=snapshot)
        
        return self._store
    
    def getStore(self)->TTreeInterface:
        """Returns the preselection summary object.

        Returns:
            TTreeInterface: preselection summary object
        """
        
        assert(self._store is not None)
        
        return self._store
    
    def plotFinalStateCounts(self):
        assert(self._rf is not None)
        from zhh import plot_final_state_counts
        
        return plot_final_state_counts(self._rf['Merged'])
    
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
        
        assert(self._rf is not None and self._store is not None)
        
        self._lumi_inv_ab = lumi_inv_ab
        
        tree = self._rf['Merged']
                
        # fetch data for weight calculation  
        process = tree['process'].array()

        weight_data = np.zeros(tree['process'].num_entries, dtype=[
            ('pid', 'H'),
            ('process', 'I'),
            ('polarization_code', 'B'),
            ('cross_section', 'f'),
            ('n_gen', 'I'),
            ('weight', 'f')])
        
        weight_data['process'] = process
        weight_data['polarization_code'] = tree['polarization_code'].array()
        weight_data['cross_section'] = tree['cross_section'].array()

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
        for proc in np.nditer(unq_processes[0]):
            proc = int(proc)
            for polarization_code in np.unique(weight_data[weight_data['process'] == proc]['polarization_code']):
                mask = (weight_data['process'] == proc) & (weight_data['polarization_code'] == polarization_code)
                
                process_name = ProcessCategories.inverted[proc]
                Pem, Pep = parse_polarization_code(polarization_code)

                cross_sec = weight_data[mask]['cross_section'][0]
                n_gen = np.sum(mask)
                wt = sample_weight(cross_sec, (Pem, Pep), n_gen, lumi_inv_ab)
                
                print(f'Process {process_name:12} with Pol e{"L" if Pem == -1 else "R"}.p{"L" if Pep == -1 else "R"} has {n_gen:9} events xsec={cross_sec:.3E} wt={wt:.3E}')
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
                
        self._store['pid'] = weight_data['pid']
        self._store['weight'] = weight_data['weight']
        self._processes = processes
                
        return weight_data, processes
    
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
    
    def getFinalStateCounts(self)->FinalStateCounts:
        """Returns the final state counts array.

        Returns:
            np.ndarray: final state counts
        """
        
        from .TTreeInterface import parse_final_state_counts
        
        assert(self._store is not None)
        
        return parse_final_state_counts(self.getStore())
    
    def registerEventCategory(self, name:str, mask_or_func:Callable|np.ndarray, category:int|None, overwrite:bool=False):
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
    
    def evaluateEventCategories(self, default_category:int|None, force:bool=False, order:Optional[list[str]]=None)->'AnalysisChannel':
        """Evaluates event category definitions and saves them as numpy masks.
        default_category is assigned to all events first, if not None.
        After that, all categories for which the category paraemter in the
        registerEventCategory() call was not None, are assigned. If order is
        an empty array, nothing is changed after the default category is assigned.  

        Args:
            default_category (int | None): _description_
            force (bool, optional): _description_. Defaults to False.
            order (Optional[list[str]], optional): if None, all categories are considered
                in the order they were registered. If a list, ONLY the contained categories
                are considered. Defaults to None.

        Returns:
            AnalysisChannel: _description_
        """
        from .TTreeInterface import parse_final_state_counts
        
        if not self._evalutatedEventCategories or force:
            print(f'Evaluating event categories for {self._name}...')
            
            presel = self.getStore()
            fsc = parse_final_state_counts(presel)

            for name, (mask_func, category) in self._event_category_resolvers.items():
                mask = mask_func(self, fsc)
                self._event_categories[name] = (mask, category)
            
            if order is None:
                order = list(self._event_categories.keys())
            
            if default_category is not None:
                presel['event_category'][:] = default_category
                print(f' Assigned default event category (ID={default_category}) to all {len(presel["event_category"]):d} events')
            else:
                print(f' No default event category assigned')
            
            mask_sum = np.zeros(len(self._event_categories[next(iter(self._event_categories))][0]), dtype='B')
            
            for i, category_name in enumerate(order):
                mask, replace = self._event_categories[category_name]
                
                print(f' Assigned event category {category_name} (ID={replace}) to {np.sum(mask):d} events with weight {np.sum(presel["weight"][mask]):3g}')
                if replace is not None:
                    presel['event_category'][mask] = replace
                
                collision = np.logical_and(mask_sum > 0, mask)
                n_collisions = np.sum(collision)
                if n_collisions > 0:
                    print(f'  Warning: Event category {category_name} is not orthogonal to previous selection. Found {n_collisions} collisions')
                
                mask_sum += mask
            
            n_assigned = np.sum(mask_sum > 0)
            
            print(f' Total: Event categories assigned to {n_assigned:d} events ({(n_assigned / len(mask_sum)):.2%})')
                
            self._evalutatedEventCategories = True
        
        return self
    
    def getCategoryMask(self, name:str):
        if name not in self._event_categories:
            if name in self._event_category_resolvers:
                from .TTreeInterface import parse_final_state_counts
                
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