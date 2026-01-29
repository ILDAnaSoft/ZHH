from ..CutflowProcessorAction import CutflowProcessorAction, CutflowProcessor

class SklearnMulticlassHyperparamTrainingAction(CutflowProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, mva:str, clf_prop:str, **kwargs):
        """_summary_

        Args:
            cp (CutflowProcessor): _description_
            steer (dict): _description_
            mva (str): _description_
        """
        assert('mvas' in steer)

        super().__init__(cp, steer)

        from zhh import find_by

        mva_spec = find_by(steer['mvas'], 'name', mva, is_dict=True)
        
        self._mva_file = mva_spec['mva_file']        
        self._file_property = clf_prop
    