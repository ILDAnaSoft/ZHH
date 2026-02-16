from zhh import DataStore
from tqdm.auto import tqdm
import numpy as np

def load_jet_matching_kinfit(store:DataStore)->np.ndarray:
    jmk = np.zeros((len(store), 4), dtype=np.int8)

    for i in range(4):
        jmk[:, i] = store[f'jet_matching_kinfit_best.dim{i}']

    return jmk

def fill_mem_momenta(store:DataStore, mask:np.ndarray|None=None, size:int|None=None):
    """H1 = Jet1 + Jet2
    Z2/H2 = Jet3 + Jet4

    Args:
        momenta (np.ndarray): _description_
        size (int): _description_
        store (DataStore): _description_

    Returns:
        _type_: _description_
    """

    if size is None:
        size = int(mask.sum()) if mask is not None else len(store)

    momenta = np.zeros((size, 24), dtype=np.float64)

    for lep_idx in range(2):
        for idx, column in enumerate(['fX', 'fY', 'fZ', 'fT']):
            prop = f'lep{lep_idx + 1}_4v.fCoordinates.fCoordinates.{column}'
            momenta[:, 4 * lep_idx + idx] = store[prop][:size] if mask is None else store[prop][mask]

    for jet_idx in range(4):
        for idx, column in enumerate(['fX', 'fY', 'fZ', 'fT']):
            prop = f'jet{jet_idx + 1}_4v.fCoordinates.fCoordinates.{column}'
            momenta[:,  8 + 4 * jet_idx + idx] = store[prop][:size] if mask is None else store[prop][mask]

    return momenta

def fill_mem_momenta_old(store:DataStore, jmk:np.ndarray|None=None,
                 mask:np.ndarray|None=None, size:int|None=None):
    """H1 = Jet1 + Jet2
    Z2/H2 = Jet3 + Jet4

    Args:
        momenta (np.ndarray): _description_
        size (int): _description_
        store (DataStore): _description_

    Returns:
        _type_: _description_
    """

    if size is None:
        size = int(mask.sum()) if mask is not None else len(store)

    momenta_zhh = np.zeros((size, 16), dtype=np.float64)
    momenta_zzh = np.zeros((size, 20), dtype=np.float64)

    if True:
        for idx, column in enumerate([
            'lep1_4v.fCoordinates.fCoordinates.fX',
            'lep1_4v.fCoordinates.fCoordinates.fY',
            'lep1_4v.fCoordinates.fCoordinates.fZ',
            'lep1_4v.fCoordinates.fCoordinates.fT',

            'lep2_4v.fCoordinates.fCoordinates.fX',
            'lep2_4v.fCoordinates.fCoordinates.fY',
            'lep2_4v.fCoordinates.fCoordinates.fZ',
            'lep2_4v.fCoordinates.fCoordinates.fT',
        ]):
            momenta_zhh[:, idx] = store[column][:size] if mask is None else store[column][mask]
            momenta_zzh[:, idx] = momenta_zhh[:, idx]

    # use JMK
    if jmk is None:
        for idx, column in enumerate(['fX', 'fY', 'fZ', 'fT']):
            prop1 = f'jet1_4v.fCoordinates.fCoordinates.{column}'
            prop2 = f'jet2_4v.fCoordinates.fCoordinates.{column}'

            momenta_zhh[:,  8 + idx] = (store[prop1][:size] if mask is None else store[prop1][mask]) + \
                                    (store[prop2][:size] if mask is None else store[prop2][mask])
            
            momenta_zzh[:, 16 + idx] = momenta_zhh[:, 8 + idx] # ZZH H momenta = ZHH H1 momenta

        for idx, column in enumerate([
            'jet3_4v.fCoordinates.fCoordinates.fX',
            'jet3_4v.fCoordinates.fCoordinates.fY',
            'jet3_4v.fCoordinates.fCoordinates.fZ',
            'jet3_4v.fCoordinates.fCoordinates.fT',

            'jet4_4v.fCoordinates.fCoordinates.fX',
            'jet4_4v.fCoordinates.fCoordinates.fY',
            'jet4_4v.fCoordinates.fCoordinates.fZ',
            'jet4_4v.fCoordinates.fCoordinates.fT'
        ]):
            momenta_zzh[:, 8 + idx] = store[column][:size] if mask is None else store[column][mask]
        
        for i in range(4):
            momenta_zhh[:, 12 + i] = momenta_zzh[:, 8 + i] + momenta_zzh[:, 12 + i]
    else:
        assert(len(jmk) == size)
        
        jet_momenta = np.zeros((size, 16), dtype=np.float64)
        entry = np.zeros(16, dtype=np.float64)

        for jet_idx in range(4):
            for idx, column in enumerate(['fX', 'fY', 'fZ', 'fT']):
                prop = f'jet{jet_idx + 1}_4v.fCoordinates.fCoordinates.{column}'
                jet_momenta[:, jet_idx * 4 + idx] = store[prop][:size] if mask is None else store[prop][mask]

        for i, matching in enumerate(jmk[:size] if mask is None else jmk): # jmk[mask]):
            if matching[0] > -1:
                for j in range(4):
                    # j: index over jet_idx

                    for k in range(4):
                        # k: index over px,py,pz,e
                        entry[j * 4 + k] = jet_momenta[i, matching[j] * 4 + k]
                
                for k in range(4):
                    momenta_zhh[i,  8 + k] = entry[    k] + entry[ 4 + k] # h1
                    momenta_zhh[i, 12 + k] = entry[8 + k] + entry[12 + k] # h2

                    momenta_zzh[i, 16 + k] = momenta_zhh[i,  8 + k] # Higgs
                    momenta_zzh[i,  8 + k] = entry[ 8 + k]
                    momenta_zzh[i, 12 + k] = entry[12 + k]
            else:
                momenta_zzh[i, :] = 0

            #if i == 2:
            #    break
        
    return momenta_zhh, momenta_zzh