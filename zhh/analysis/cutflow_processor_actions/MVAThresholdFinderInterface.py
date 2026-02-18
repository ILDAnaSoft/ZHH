from ..CutflowProcessorAction import CutflowProcessorAction, CutflowProcessor
from abc import abstractmethod

class MVAThresholdFinderInterface(CutflowProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, **kwargs):
        """Interface for an action that assigns a threshold parameter to a MVA in a CutflowProcessor

        Args:
            cp (CutflowProcessor): _description_
            steer (dict): _description_
            mva (str): name of the MVA this action should assign a threshold to
        """

        mva = kwargs['mva']

        from zhh import find_by

        assert('mvas' in steer)
        mva_spec = find_by(steer['mvas'], 'name', mva, is_dict=True)        

        super().__init__(cp, steer)

        self._mva_name = mva
    
    @abstractmethod
    def findThreshold(self)->float:
        """Method to be implemented by an implementing action. Should return the decision
        threshold used to divide predictions into signal/background. I.e. values >=
        threshold are signal, else background. Should be executed in the complete()
        method of the inheriting action.

        Raises:
            NotImplementedError: _description_

        Returns:
            float: _description_
        """
        raise NotImplementedError('assignThreshold must be implemented')
    
    def assignThreshold(self, value:float|None=None):
        self._cp._mvas[self._mva_name]['threshold'] = self.findThreshold() if value is None else value