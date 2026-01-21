from ..CutflowProcessorAction import CutflowProcessorAction, CutflowProcessor
from zhh import find_by

class InferenceSklearnMulticlassAction(CutflowProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, use:str, **kwargs):
        """_summary_

        Args:
            cp (CutflowProcessor): _description_
            steer (dict): _description_
            use (str): _description_
            from_file (str): _description_
            from_file_property (str, optional): _description_. Defaults to 'clf'.
        """
        assert('mvas' in steer)

        super().__init__(cp, steer)

        mva_spec = find_by(steer['mvas'], 'name', use, is_dict=True)
        
        self._file = from_file        
        self._file_property = from_file_property
    