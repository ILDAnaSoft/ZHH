from ..CutflowProcessorAction import CutflowProcessorAction, CutflowProcessor

class InferenceSklearnMulticlassAction(CutflowProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, use:str, from_file:str, from_file_property:str='clf', **kwargs):
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
        
        self._file = from_file        
        self._file_property = from_file_property
    