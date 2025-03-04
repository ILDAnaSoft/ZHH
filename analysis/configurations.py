from collections.abc import Callable, Sequence
from zhh import zhh_cuts, Cut, EqualCut, fetch_preselection_data
import uproot as ur
import os.path as osp
import os, subprocess, numpy as np

# 4x3 analysis channels:
# llHH:
# vHH:
# light qqHH dominant, bbHH dominant: 

class AnalysisChannel:
    def __init__(self, name:str, cuts:Sequence[Cut], define_bkg:Callable, define_sig:Callable):        
        self._name = name
        self._cuts = cuts
        self._root_files = []
        self._define_bkg = define_bkg
        self._define_sig = define_sig
        self.rf = None
        
    def initialize(self, work_root:str, root_files:list[str]):
        self._work_dir = f'{work_root}/{self._name}'
        self._root_files = root_files
        
        if not os.path.exists(self._work_dir):
            os.makedirs(self._work_dir)
        
        if not os.path.isfile(f'{self._work_dir}/source.root'):
            cmd = f'source {os.environ["REPO_ROOT"]}/setup.sh && hadd -f source.root ' + ' '.join(self._root_files)
            print(cmd)
            p = subprocess.Popen(cmd, shell=True, cwd=self._work_dir)
            if p.wait() != 0:
                raise RuntimeError('Failed to merge root files')
            
        if self.rf is None:
            self.rf = ur.open(f'{self._work_dir}/source.root')
    
    def fetch(self):
        self.presel = fetch_preselection_data(self.rf)
    
    def weight(self):
        pass
    
        weights = np.zeros(len(pids), dtype=[('pid', 'H'), ('weight', 'f')])
        weights['pid'] = pids

        for pid in tqdm(pids):
            process = processes[processes['pid'] == pid][0]
            pol_em, pol_ep = process['pol_e'], process['pol_p']
            cross_sec = process['cross_sec']
            
            n_gen = np.sum(presel_results['pid'] == pid)
            
            weights['weight'][weights['pid'] == pid] = sample_weight(cross_sec, (pol_em, pol_ep), n_gen)
        
    def run_tmva(self, properties:list[str], train_test_ratio:float=0.2):
        self._properties = properties
        self._train_test_ratio = train_test_ratio
    
    def presel(self):
        assert(self.rf is not None)
        
        masks = []
        efficiencies
        
        mask = np.ones(self.rf['FinalStates'].num_entries, dtype=bool)
        
        self._mask_presel = None
    
    def prepare(self, root_file:ur.ReadOnlyFile):
        self._mask_sig:np.ndarray = self._define_sig(root_file)
        self._mask_bkg:np.ndarray = self._define_bkg(root_file)
        
llhh1_lvbbqq = AnalysisChannel('llhh(eehh)_lvbbqq', zhh_cuts('llhh') + [EqualCut('ll_dilepton_type', 11)],
                               define_bkg=lambda a: True,
                               define_sig=lambda b: True)