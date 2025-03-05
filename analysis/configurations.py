from collections.abc import Callable, Sequence
from zhh import zhh_cuts, Cut, EqualCut, fetch_preselection_data, sample_weight, parse_polarization_code, \
    ProcessCategories
import uproot as ur
import os.path as osp
import os, subprocess, ROOT, numpy as np

# 4x3 analysis channels:
# llHH:
# vHH:
# light qqHH dominant, bbHH dominant: 

config = {
    'N_CORES': 8
}

class AnalysisChannel:
    def __init__(self, name:str, cuts:Sequence[Cut], define_bkg:Callable, define_sig:Callable):     
        #ROOT.EnableImplicitMT(config['N_CORES'])
           
        self._name = name
        self._cuts = cuts
        self._root_files = []
        self._define_bkg = define_bkg
        self._define_sig = define_sig
        self.rf = None
        
    def initialize(self, work_root:str, root_files:list[str], trees:list[str]):
        self._work_dir = f'{work_root}/{self._name}'
        self._root_files = root_files
        self._merged_file = merged_file = f'{self._work_dir}/Merged.root'
        
        if not os.path.exists(self._work_dir):
            os.makedirs(self._work_dir)
        
        if not os.path.isfile(merged_file):
            if False:
                cmd = f'source {os.environ["REPO_ROOT"]}/setup.sh && hadd -f source.root ' + ' '.join(self._root_files)
                print(cmd)
                p = subprocess.Popen(cmd, shell=True, cwd=self._work_dir)
                if p.wait() != 0:
                    raise RuntimeError('Failed to merge root files')
            elif False:
                with open(f'{self._work_dir}/merge.C', 'w') as f:
                    f.write('TChain *chain = new TChain("hAnl");\n')
                    for i, path in enumerate(self._root_files):
                        f.write(f'chain->Add("{path}");\n')
                    f.write(f'chain->Merge("{self._work_dir}");\n')
            else:
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
            
        if self.rf is None:
            self.rf = ur.open(self._merged_file)
    
    def fetch(self, presel:str, tree:str='Merged'):
        self.summary = fetch_preselection_data(self.rf, presel, tree=tree)
    
    def weight(self, lumi_inv_ab:float=2.)->tuple[np.ndarray,np.ndarray]:
        assert(self.rf is not None)
        tree = self.rf['Merged']
                
        # fetch data for weight calculation  
        process = tree['process'].array()

        weight_data = np.zeros(tree['process'].num_entries, dtype=[
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

        processes = np.zeros(n_combinations, dtype=[('pid', 'H'), ('process', '<U60'), ('pol_e', 'i'), ('pol_p', 'i'), ('polarization_code', 'B'), ('cross_sec', 'f'), ('n_events', 'I'), ('weight', 'f')])
        processes['pid'] = np.arange(n_combinations)

        counter = 0
        for proc in np.nditer(unq_processes[0]):
            proc = int(proc)
            for polarization_code in np.unique(weight_data[weight_data['process'] == proc]['polarization_code']):
                process_name = ProcessCategories.inverted[proc]
                Pem, Pep = parse_polarization_code(polarization_code)

                cross_sec = weight_data[(weight_data['process'] == proc) & (weight_data['polarization_code'] == polarization_code)]['cross_section'][0]
                n_gen = np.sum((weight_data['process'] == proc) & (weight_data['polarization_code'] == polarization_code))
                wt = sample_weight(cross_sec, (Pem, Pep), n_gen, lumi_inv_ab)
                
                print(f'Process {process_name:12} with Pol e{"L" if Pem == -1 else "R"}.p{"L" if Pep == -1 else "R"} has {n_gen:9} events xsec={cross_sec:.3E} wt={wt:.3E}')
                
                processes[counter]['process'] = process_name
                processes[counter]['pol_e'] = Pem
                processes[counter]['pol_p'] = Pep
                processes[counter]['polarization_code'] = polarization_code
                processes[counter]['cross_sec'] = cross_sec
                processes[counter]['n_events'] = n_gen
                processes[counter]['weight'] = wt
                
                weight_data['weight'][(weight_data['process'] == proc) & (weight_data['polarization_code'] == polarization_code)] = wt
                
                counter += 1
                
        return weight_data, processes
    
    def presel(self):
        assert(self.rf is not None)
        
        self.masks = masks = []
        # efficiencies
        
        mask = np.ones(self.rf['FinalStates'].num_entries, dtype=bool)
        
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