from ..util.LazyTablelike import MixedLazyTablelike
import numpy as np
import uproot as ur
import h5py
import os.path as osp
from dataclasses import dataclass
from tqdm.auto import tqdm
from math import ceil

dtypes = {
    'id': 'I',
    'process':  'I', # H=np.uint16
    'pid': 'I',
    'pol_code': 'B', # np.uint8
    'event': 'I', # max. encountered: 15 797 803 << 4 294 967 295 (max of uint32)
    'event_category': 'B', # np.uint8
    'n_b_from_higgs': 'B',
    
    'is_sig': '?',
    'is_bkg': '?',
    
    'thrust': 'f',
    'evis': 'f',
    'ptmiss': 'f',
    'm_miss': 'f',
    'nisoleptons': 'B',
    'xx_paired_isoleptype': 'B',
    
    'passed': 'B',
    'weight': 'f',
    
    'bmax1': 'f',
    'bmax2': 'f',
    'bmax3': 'f',
    'bmax4': 'f',
    
    'zhh_mh1': 'f',
    'zhh_mh2': 'f',
    
    # ll
    'dilepton_type': 'B',
    'mzll': 'f',
    'mz_pre_pairing': 'f',
    
    # vv
    'zhh_mhh': 'f',
}


class TTreeInterface(MixedLazyTablelike):
    cache:dict[str, np.ndarray] = {}
    
    def __init__(self, tree:ur.TTree, final_states:bool=True,
                 prop_prefix:str='', in_memory:bool=False,
                 n_jets:int=4, full_final_states:bool=False,
                 snapshot:str|None=None, snapshot_open_mode:str='r'):
        import awkward as ak
        
        r_size = tree.num_entries
        super().__init__(r_size)
        
        def fetch(key:str, loc:str|None=None):
            if loc is None:
                loc = prop_prefix + key
                
            if in_memory:
                if key in self.cache:
                    return self.cache[key]
                else:
                    self.cache[key] = np.array(tree[loc].array(), dtype=dtypes[key] if key in dtypes else 'f')
                    return self.cache[key]
            else:
                # make dtype default to float
                return np.array(tree[loc].array(), dtype=dtypes[key] if key in dtypes else 'f')
        
        # this attaches the properties of the TTree 
        self._defaultHandler = fetch
        self._snapshot:str|None = None
        
        # readonly
        self['process'] = lambda intf: fetch('process')
        self['pol_code'] = lambda intf: fetch('pol_code', 'polarization_code')
        
        self['event'] = lambda intf: np.array(tree['event'].array(), dtype='I')
        
        use_snapshot = snapshot is not None and osp.isfile(f'{snapshot}.npz') and osp.isfile(f'{snapshot}.h5')
            
        if final_states and not use_snapshot:
            from .PreselectionAnalysis import fs_columns
            
            if full_final_states:
                self['Nb_from_H'] = lambda intf: fetch('n_b_from_higgs')
                
                def fs_counts(column:int|None):
                    if column is not None:
                        return np.array(tree['final_state_counts.second'].array(array_cache=None)[:, column], dtype='B')
                    else:
                        return np.array(tree['final_state_counts.second'].array(array_cache=None), dtype='B')
                
                def attach_fs_counts(obj, attr, data):
                    obj[attr] = data
                
                fsc = fs_counts(None)
                for i in range(len(fs_columns)):
                    attach_fs_counts(self, fs_columns[i], fsc[:, i])
            else:
                length = tree.num_entries

                data = { 'Nb_from_H': np.zeros(length, dtype='B') }
                props = ['Nd', 'Nu', 'Ns', 'Nc', 'Nb', 'Nt', 'Nv1', 'Nv2', 'Nv3', 'Ne1', 'Ne2', 'Ne3']
                idx = [fs_columns.index(prop) for prop in props]

                for prop in props:
                    data[prop] = np.zeros(length, dtype='B')
                    
                pointer = 0
                chunk_size = int(1e7)
                pbar = tqdm(range(ceil(length / chunk_size)))
                
                while pointer < length:
                    actual_size = min(chunk_size, length - pointer)
                    
                    data['Nb_from_H'][pointer:pointer+actual_size] = tree['n_b_from_higgs'].array(array_cache=None, entry_start=pointer, entry_stop=pointer+actual_size)
                    
                    chunk = tree['final_state_counts.second'].array(array_cache=None, entry_start=pointer, entry_stop=pointer+actual_size)[:, idx]
                    for i, prop in enumerate(props):
                        data[prop][pointer:pointer+actual_size] = chunk[:, i]
                    
                    pointer += actual_size
                    pbar.update(1)
                    
                pbar.close()
                    
                #data['Nq'] = data['Nd'] + data['Nu'] + data['Ns'] + data['Nc'] + data['Nb'] + data['Nt']
                #data['NneutralLep'] = data['Nv1'] + data['Nv2'] + data['Nv3']
                #data['NchargedLep'] = data['Ne1'] + data['Ne2'] + data['Ne3']
                
                for prop in data:
                    self[prop] = data[prop]
        
        # NEW: explicit definition of properties inside TTrees that are of type float are no more necessary
        #self['zhh_mh1'] = lambda intf: fetch('zhh_mh1', f'{prop_prefix}zhh_mh1')
        #self['zhh_mh2'] = lambda intf: fetch('zhh_mh2', f'{prop_prefix}zhh_mh2')
        
        self['mz_pre_pairing'] = lambda intf: fetch('mz_pre_pairing', f'{prop_prefix}mzll_pre_pairing')
        self['mhh'] = lambda intf: fetch('invJetMass', f'{prop_prefix}invJetMass')
        self['sumBTags'] = lambda intf: ( fetch('bmax1') + fetch('bmax2') + fetch('bmax3') + fetch('bmax4') )
        
        # REMOVE ALL once fixed in EventObservables!
        if False:
            masses = tree['fit4C_masses'].array()
            mask = np.array(ak.count(masses, axis=1) == 2, dtype=bool)
            
            fit4C_mh1 = np.zeros(len(masses))
            fit4C_mh2 = np.zeros(len(masses))
            
            fit4C_mh1[mask] = ak.min(masses, axis=1)[mask]
            fit4C_mh2[mask] = ak.max(masses, axis=1)[mask]
            
            self['fit4C_mh1'] = fit4C_mh1
            self['fit4C_mh2'] = fit4C_mh2
            
            bTags = np.array(tree['bTags'].array())
            bTags = -np.sort(-bTags, axis=1)

            self['bmax1'] = bTags[:, 0]
            self['bmax2'] = bTags[:, 1]
            self['bmax3'] = bTags[:, 2]
            self['bmax4'] = bTags[:, 3]
        
        self['yminus_mod100'] = lambda intf: np.mod(intf['yminus'], 100)
        self['cosjzmax'] = lambda intf: np.stack([
            intf['cosJ1Z_2Jets'],
            intf['cosJ2Z_2Jets']
        ]).max(axis=0)
        
        if use_snapshot:
            assert(isinstance(snapshot, str))
            self._snapshot = snapshot
            self.propsFromSnapshot(snapshot)
        else:
            self.itemsFromTree(tree, n_jets)            
    
    def itemsFromTree(self, tree, n_jets:int):
        # writable
        self['id'] = np.arange(len(self), dtype=dtypes['id'])
        self['event_category'] = np.array(tree['event_category'].array(), dtype='B')
        
        # writable; attached by DataSource.weight()
        self['pid'] = np.nan*np.ones(len(self), dtype='I')
        self['weight'] = np.nan*np.ones(len(self), dtype='f')
        
        jet_masses = np.zeros((len(self), n_jets), dtype='f')

        for i in range(n_jets):
            print(f'Fetching jet{i}_m')
            jet_masses[:, i] = self[f'jet{i+1}_m']
            
        jet_masses.sort(axis=1)
        
        for i in range(n_jets):
            print(f'Assigning jet{i}_m')
            self[f'jet{i+1}_m'] = jet_masses[:, i]
    
    def propsFromSnapshot(self, bname:str):
        """Fetches data from cache files under f'{bname}.h5' attaches them as props to
        the TTreeInterface. Used together with itemsSnapshot

        Args:
            bname (str): _description_
        """
               
        keys = []
        with h5py.File(f'{bname}.h5', 'r') as hf:
            for key in hf.keys():
                keys.append(key)

        for item in tqdm(keys):
            #if item in fs_columns:
                self[item] = mk_ref(bname, item)
            #else:
            #    self[item] = hf[item][:]

        # to be deprecated
        if osp.isfile(f'{bname}.npz'):
            npz = np.load(f'{bname}.npz')
            for prop in npz:
                self[prop] = npz[prop]
    
    def itemsSnapshot(self, bname:str, additional:dict={}):
        """Saves a snapshot of all registered items in a HDF5 file under f'{bname}.h5'.

        Deprecated: Objects which are no properties of the TTreeInterface but should still be saved
        together can be given in key/value notation in the dict additional. Used together
        with propsFromSnapshot

        Args:
            bname (str): _description_
            additional (dict, optional): to be deprecated. Defaults to {}.
        """
        
        if len(additional.keys()):
            print(f'Warning: Using additional is deprecated and will be removed soon.')
            np.savez_compressed(f'{bname}.npz', **additional)
        
        print(f'Writing cache file {bname}.h5')
        with h5py.File(f'{bname}.h5', 'w') as hf:
            for item, value in (pbar := tqdm(self._items.items())):
                pbar.set_description(f'Processing {item}...')                
                dset = hf.create_dataset(item, value.shape, dtype=value.dtype, compression='gzip')
                dset[:] = value
    
    def addItemToSnapshot(self, name:str, arr_or_None:np.ndarray|None=None, overwrite:bool=False):
        """Adds a named entry to the snapshot/cache HDF5 file. The value to be added
        must either be passed or will be read from the interface at the specified name
        (in this case, the current view will be used. make sure to call resetView()
        beforehand). Must be called within a context created by
        prepareSnapshotTransaction().

        Args:
            name (str): _description_
            arr_or_None (np.ndarray | None, optional): _description_. Defaults to None.

        Raises:
            Exception: _description_
        """
        
        bname = self._snapshot
        
        if not isinstance(bname, str) or not osp.isfile(f'{bname}.h5'):
            raise Exception('Cache must already exist to append items')
        
        value = arr_or_None if arr_or_None is not None else self[name]
        
        with h5py.File(f'{bname}.h5', mode='a') as hf:
            if name in hf.keys() and not overwrite:
                print(f'Property {name} already exists in snapshot and will be skipped (i.e. NOT saved in the cache)')
            else:
                dset = hf.create_dataset(name, value.shape, dtype=value.dtype, compression='gzip')
                dset[:] = value
                
        self[name] = mk_ref(bname, name)
        

def mk_ref(bname, item):
    def ref(intf):
        with h5py.File(f'{bname}.h5', 'r') as hf:
            return hf[item][:]
            
    return ref

@dataclass
class FinalStateCounts:
    n_d: np.ndarray
    n_u: np.ndarray
    n_s: np.ndarray
    n_c: np.ndarray
    n_b: np.ndarray
    n_t: np.ndarray
    n_q: np.ndarray
    
    n_ve: np.ndarray
    n_vmu: np.ndarray
    n_vtau: np.ndarray
    n_neutral_lep: np.ndarray
    
    n_e: np.ndarray
    n_mu: np.ndarray
    n_tau: np.ndarray
    n_charged_lep: np.ndarray
    
    n_b_from_higgs: np.ndarray

def parse_final_state_counts(store:TTreeInterface)->FinalStateCounts:
    from zhh import PDG2FSC
    
    store.resetView()
    n_b_from_higgs = store['Nb_from_H']
    
    n_d = store['Nd']
    n_u = store['Nu']
    n_s = store['Ns']
    n_c = store['Nc']
    n_b = store['Nb']
    n_t = store['Nt']
    n_q = n_d + n_u + n_s + n_c + n_b + n_t
    
    n_ve   = store['Nv1']
    n_vmu  = store['Nv2']
    n_vtau = store['Nv3']
    n_neutral_lep = n_ve + n_vmu + n_vtau
    
    n_e   = store['Ne1']
    n_mu  = store['Ne2']
    n_tau = store['Ne3']
    n_charged_lep = n_e + n_mu + n_tau
    
    return FinalStateCounts(
        n_d=n_d,
        n_u=n_u,
        n_s=n_s,
        n_c=n_c,
        n_b=n_b,
        n_t=n_t,
        n_q=n_q,
        
        n_ve=n_ve,
        n_vmu=n_vmu,
        n_vtau=n_vtau,
        n_neutral_lep=n_neutral_lep,
        
        n_e=n_e,
        n_mu=n_mu,
        n_tau=n_tau,
        n_charged_lep=n_charged_lep,
        n_b_from_higgs=n_b_from_higgs
    )
