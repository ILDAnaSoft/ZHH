import uproot as ur
import numpy as np
from collections.abc import Callable
from ..analysis.AnalysisChannel import AnalysisChannel

class ROOTFeatureProvider:
    def __init__(self, branches:list[str]):
        """Takes in a list of [<SourceTTree>.]<Branch>
        entries and resolves them to existing branches
        within a ROOT TTree in the resolve method. If 
        SourceTTree is given, a branch starting with
        <SourceTTree>_ is optionally searched for.

        Args:
            branches (list[str]): _description_
        """
        self._input_branches = branches
        self._resolved_branches = []
        self._resolved = False
        self._mapping = {}
        self._analysis:AnalysisChannel|None = None
        self._registered_features = {}
    
    def resolve(self, analysis:AnalysisChannel):        
        if not self._resolved:
            tree = analysis.getTTree()
                
            properties:list[str] = tree.keys()
        
            for i_branch, branch in enumerate(self._input_branches):
                containing_tree = None
                if '.' in branch:
                    split = branch.split('.')
                    assert(len(split) == 2)
                    containing_tree, branch = split
                
                if branch in properties:
                    self._resolved_branches.append(branch)
                else:
                    # iterate through all properties, check if _{branch} exists
                    found = False
                    for prop in properties:
                        #if (prop.endswith(f'_{branch}') if containing_tree is None else (fnmatch.fnmatch(prop, f'{containing_tree}*[_]{branch}'))):
                        if prop.endswith(f'_{branch}') and (True if containing_tree is None else prop.startswith(containing_tree)):
                            found = True
                            self._resolved_branches.append(prop)
                            break
                        
                    if not found:
                        raise Exception(f'Could not resolve branch <{branch}>')
            
            self._analysis = analysis
            self._resolved = True
            for i_branch in range(len(self._input_branches)):
                self._mapping[self._input_branches[i_branch]] = self._resolved_branches[i_branch]
                
        return self._resolved_branches
    
    def __getitem__(self, key):
        assert(self._resolved and self._analysis is not None)
        
        if key in self._resolved_branches:
            return np.array(self._analysis.getTTree()[key].array())
        elif key in self._registered_features:
            return self._registered_features[key](self._analysis.getTTree())
        else:
            raise Exception(f'Feature {key} not registered')
    
    def setSource(self, analysis:AnalysisChannel):
        assert(self._resolved)
        
        keys = analysis.getTTree()
        assert(all([branch in keys for branch in self._resolved_branches]))
        
        self._analysis = analysis
    
    def registerFeature(self, name, fn:Callable[[ur.TTree], bool]):
        self._registered_features[name] = fn