from collections.abc import Callable, Sequence
from .Cuts import Cut, EqualCut
from .ZHHCuts import zhh_cuts
from .PreselectionAnalysis import fetch_preselection_data, sample_weight, get_pol_key
from .PreselectionSummary import PreselectionSummary
from zhh.processes import parse_polarization_code, ProcessCategories

import uproot as ur
import os.path as osp
import os, subprocess, numpy as np
from tqdm.auto import tqdm
from typing import cast, Optional

config = {
    'N_CORES': 8
}

class AnalysisChannel:
    def __init__(self, work_root:str, name:str='', define_bkg:Callable|None=None, define_sig:Callable|None=None, cuts:Optional[Sequence[Cut]]=None):
        """Helper class to combine ROOT files+TTrees, calculate weights
        and prepare data for TMVA.

        Args:
            work_root (str): directory to store Merged.root. must be user writable
            name (str, optional): _description_. Defaults to ''.
            define_bkg (Callable | None, optional): _description_. Defaults to None.
            define_sig (Callable | None, optional): _description_. Defaults to None.
            cuts (Optional[Sequence[Cut]], optional): _description_. Defaults to None.
        """
        
        import ROOT
        if config['N_CORES'] is not None:
            ROOT.EnableImplicitMT(config['N_CORES'])
        
        self._work_root = work_root
        
        self._name = name
        self._root_files = []
        self._merged_file = ''
        
        self._define_bkg = define_bkg
        self._define_sig = define_sig
        self._rf:ur.WritableFile|None = None
        self._summary:PreselectionSummary|None = None
        self._cuts = cuts
        
    def combine(self, trees:list[str], root_files:list[str]|None=None):
        """Combines all ROOT TTrees in root_files into one "Merged" TTree in Merged.root
        inside work_root using TChain with RDataFrame and its snapshot method. If there
        are branches with the same name in different trees, the first is saved under the
        usual name, and all subsequent ones with their original tree name as prefix. 

        Args:
            trees (list[str]): name of all ROOT TTree objects to merge
            root_files (list[str] | None, optional): paths of all ROOT files to merge
        """
        
        import ROOT

        self._root_files = root_files
        self._merged_file = merged_file = f'{self._work_root}/Merged.root'
        
        if not os.path.exists(self._work_root):
            os.makedirs(self._work_root)
        
        if not os.path.isfile(merged_file):
            assert(isinstance(root_files, list) and isinstance(trees, list))
            
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
            df.Snapshot('Merged', merged_file)
            
        if self._rf is None:
            self._rf = cast(ur.WritableFile, ur.open(self._merged_file))
    
    def fetch(self, presel:str, tree:str='Merged'):
        assert(self._rf is not None)
        
        self._summary = PreselectionSummary(cast(ur.TTree, self._rf[tree]), preselection=presel)
        #self._summary = fetch_preselection_data(self._rf, presel, tree=tree)
        return self._summary
    
    def weight(self, lumi_inv_ab:float=2.)->tuple[np.ndarray,np.ndarray]:
        """Extracts cross-sections and number of generated events for each
        polarization-process combination. Then calculates weights for each
        combination (processes, n x M) and event (weight_data, l x K). The
        results are given as two named numpy arrays.
        
        n: number of process-polarization ("proc_pol") combinations
        M: 9 features, e.g. "cross_sec", "proc_pol", "n_events", "weight"
        
        l: number of events in Merged.root
        K: 6 features, e.g. "weight", "process", "polarization_code"

        Args:
            lumi_inv_ab (float, optional): _description_. Defaults to 2..

        Returns:
            tuple[np.ndarray,np.ndarray]: weight_data, processes
        """
        
        assert(self._rf is not None and self._summary is not None)
        
        tree = self._rf['Merged']
                
        # fetch data for weight calculation  
        process = tree['process'].array()

        weight_data = np.zeros(tree['process'].num_entries, dtype=[
            ('pid', 'I'),
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
            ('pid', 'H'),
            ('process', '<U60'),
            ('proc_pol', '<U64'),
            ('pol_e', 'i'),
            ('pol_p', 'i'),
            ('polarization_code', 'B'),
            ('cross_sec', 'f'),
            ('n_events', 'I'),
            ('weight', 'f')])
        processes['pid'] = np.arange(n_combinations)

        pid = 0
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
                
                processes[pid]['process'] = process_name
                processes[pid]['proc_pol'] = procpol
                processes[pid]['pol_e'] = Pem
                processes[pid]['pol_p'] = Pep
                processes[pid]['polarization_code'] = polarization_code
                processes[pid]['cross_sec'] = cross_sec
                processes[pid]['n_events'] = n_gen
                processes[pid]['weight'] = wt
                
                weight_data['weight'][mask] = wt
                weight_data['pid'][mask] = pid
                
                pid += 1
        
        self._summary['pid'] = weight_data['pid']
        self._summary['weight'] = weight_data['weight']
                
        return weight_data, processes
    
    def presel(self):
        assert(self._rf is not None)
        
        self.masks = masks = []
        # efficiencies
        
        mask = np.ones(self._rf['FinalStates'].num_entries, dtype=bool)
        
    def run_tmva(self, properties:list[str], train_test_ratio:float=0.2):
        self._properties = properties
        self._train_test_ratio = train_test_ratio
        
        self._mask_presel = None
    
    def prepare(self, root_file:ur.ReadOnlyFile):
        self._mask_sig:np.ndarray = self._define_sig(root_file)
        self._mask_bkg:np.ndarray = self._define_bkg(root_file)
        
llhh1_lvbbqq = AnalysisChannel('llhh_lvbbqq', zhh_cuts('llhh'),# + [EqualCut('ll_dilepton_type', 11)],
                               define_bkg=lambda a: True,
                               define_sig=lambda b: True)