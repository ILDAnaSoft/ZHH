import h5py
import os.path as osp
from typing import TypedDict, NotRequired

class Interpretation(TypedDict):
    name: str
    tree: str
    dtype: NotRequired[str]

def translate_item(sources:list[str], tree:str, name:str):
    import ROOT, awkward as ak

    chain = ROOT.TChain(tree)

    for file in sources:
        chain.Add(file)

    df = ROOT.RDataFrame(chain)
    
    return ak.from_rdataframe(df, columns=name)

class ROOT2HDF5Converter:
    def __init__(self, sources:list[str], target:str, vds_dir:str|None='vds'):
        assert(target.endswith('.h5') or target.endswith('.hdf5'))

        self._sources = sources
        self._target = target

        if vds_dir is not None and not osp.isabs(vds_dir):
            vds_dir = osp.join(osp.dirname(target), vds_dir)

        self._vds_dir = vds_dir
    
    def interpret(self, interpretations:list[Interpretation]):
        keys = []
        if osp.isfile(self._target):
            with h5py.File(self._target, mode='r') as hf:
                keys = hf.keys()

        for interpretation in interpretations:
            prop_name = f'{interpretation["tree"]}.{interpretation["name"]}'
            if prop_name in keys:
                continue
                # print(f'Property <{prop_name}> already found, will be skipped')
            else:
                if self._vds_dir is not None:
                    # output to: self._vds_dir/<prop_name>.h5
                    if osp.isfile(f'{self._vds_dir}/{prop_name}.h5'):
                        # TODO: integrity check
                        continue
                else:
                    # output to: self._target
                    with h5py.File(self._target, mode='w') as hf:
                        hf[prop_name]