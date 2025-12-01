from ..util.LazyTablelike import MixedLazyTablelike
import numpy as np
import h5py
import os.path as osp
from dataclasses import dataclass
from tqdm.auto import tqdm
from math import ceil
from typing import cast
from .TTreeInterface import FinalStateCounts

class DataStore(MixedLazyTablelike):
    cache:dict[str, np.ndarray] = {}
    
    def __init__(self, h5_file:str, final_states:bool=True,
                 in_memory:list[str]=[],
                 in_memory_writable:list[str]=[],
                 n_jets:int=4):
        
        init_done = False

        with h5py.File(h5_file) as hf:
            r_size = int(hf.attrs.get('nrows', 0))
            init_done = bool(hf.attrs.get('init_done', False))
        
        assert(r_size)
        super().__init__(r_size)
        
        def fetch(key:str):                
            if key in self._in_memory:
                if key in self.cache:
                    return self.cache[key]
                else:
                    self.cache[key] = self.fromFile(key)
                    return self.cache[key]
            else:
                # take value and dtype from HDF5
                return self.fromFile(key)
        
        # this attaches the properties of the TTree 
        self._defaultHandler = fetch
        self._h5_file = h5_file
        self._in_memory = in_memory
        
        # readonly
        self['process'] = lambda intf: fetch('process')
        self['event'] = lambda intf: fetch('event')
            
        if final_states:
            from .PreselectionAnalysis import fs_columns
            
            def attach_fs_counts(obj, attr, data):
                obj[attr] = data
            
            for i in range(len(fs_columns)):
                #attach_fs_counts(self, fs_columns[i], self.fromFile(f'final_state_counts.dim{i}'))
                self[fs_columns[i]] = mk_ref(self._h5_file, f'final_state_counts.dim{i}')
        
        # NEW: explicit definition of properties inside TTrees that are of type float are no more necessary
        #self['zhh_mh1'] = lambda intf: fetch('zhh_mh1', f'{prop_prefix}zhh_mh1')
        #self['zhh_mh2'] = lambda intf: fetch('zhh_mh2', f'{prop_prefix}zhh_mh2')
        
        self['sumBTags'] = lambda intf: ( self['bmax1'] + self['bmax2'] + self['bmax3'] + self['bmax4'] )

        self['yminus_mod100'] = lambda intf: np.mod(intf['yminus2'], 100)
        self['cosjzmax'] = lambda intf: np.stack([
            intf['cosJ1Z_2Jets'],
            intf['cosJ2Z_2Jets']
        ]).max(axis=0)
        
        if not init_done:
            self.itemsInitialize(n_jets)
        else:
            self.propsFromFile()

        if len(in_memory_writable):
            for item in in_memory_writable:
                self[item] = self.fromFile(item)[:]

    def fromFile(self, prop:str)->np.ndarray:        
        with h5py.File(self._h5_file) as hf:
            return cast(h5py.Dataset, hf[prop])[:]
    
    def itemsInitialize(self, n_jets:int):
        # writable
        self['event_category'] = np.nan*np.ones(len(self), dtype='B')
        
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

        # mark init as done
        with h5py.File(self._h5_file, 'a') as hf:
            hf.attrs['init_done'] = True
    
    def propsFromFile(self):
        """Fetches data from the HDF5 file and attaches them as props to the DataStore
        for lazy-loading. Used together with itemsSnapshot
        """
               
        keys = []
        with h5py.File(self._h5_file, 'r') as hf:
            for key in hf.keys():
                keys.append(key)
    
    def itemsSnapshot(self, overwrite:bool=False, items:list[str]|None=None):
        """Saves a snapshot of all or a list of registered items to the HDF5 file.

        Args:
            overwrite (bool, optional): if False, each time a value would be overwritten,
                                        the used will be asked for confirmation via input().
                                        if True, will overwrite without further confirmation.
                                        Defaults to False.
            items (list[str] | None, optional): if None, all items are selected for saving.
                                                if a list of strings, only these are selected.
                                                Defaults to None.

        Raises:
            Exception: _description_
        """
        
        if items is None:
            items = list(self._items.keys())
            
        print(f'Writing items <{", ".join(items)}> to file {self._h5_file}.h5')
        
        with h5py.File(self._h5_file, 'a') as hf:
            for item in (pbar := tqdm(items)):
                value = self[item]
                pbar.set_description(f'Processing {item}...')

                if item in hf.keys():
                    cont = 'y' if overwrite else input(f'Item <{item}> already exists. ' +
                                                       'Should it be overwritten? (y=yes, n=skip, a=abort)')
                    match cont.lower():
                        case 'y':
                            print(f'Overwriting <{item}>')
                            del hf[item]
                        case 'n':
                            print(f'Skipping <{item}>')
                            continue
                        case _:
                            raise Exception(f'Item <{item}> already exists')
                pass

                dset = hf.create_dataset(item, value.shape, dtype=value.dtype, compression='gzip')
                dset[:] = value
                    
    
    def addItemToSnapshot(self, name:str, arr_or_None:np.ndarray|None=None, overwrite:bool=False):
        """Adds a named entry to the HDF5 file. The value to be added must either be
        passed or will be read from the interface at the specified name (in this case,
        the current view will be used. make sure to call resetView() beforehand).

        Args:
            name (str): _description_
            arr_or_None (np.ndarray | None, optional): _description_. Defaults to None.

        Raises:
            Exception: _description_
        """
        
        value = arr_or_None if arr_or_None is not None else self[name]
        
        with h5py.File(self._h5_file, mode='a') as hf:
            if name in hf.keys() and not overwrite:
                print(f'Property {name} already exists in snapshot and will be skipped (i.e. NOT saved in the cache)')
            else:
                dset = hf.create_dataset(name, value.shape, dtype=value.dtype, compression='gzip')
                dset[:] = value
                
        self[name] = mk_ref(self._h5_file, name)
        
def mk_ref(fname, item):
    def ref(intf):
        with h5py.File(fname, 'r') as hf:
            return cast(h5py.Dataset, hf[item])[:]
            
    return ref

def parse_final_state_counts(store:DataStore)->FinalStateCounts:    
    store.resetView()
    n_b_from_higgs = store['n_b_from_higgs']
    
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