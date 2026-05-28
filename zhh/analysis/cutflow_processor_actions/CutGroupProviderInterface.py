from ..CutflowProcessorAction import CutflowProcessorAction, CutflowProcessor
from abc import ABC, abstractmethod
from ..Cuts import ValueCut

class CutGroupProviderInterface(CutflowProcessorAction, ABC):
    def __init__(self, cp:CutflowProcessor, steer:dict, cut_group:str, step:int, **kwargs):
        """Interface for an action that lazily returns a list of cuts (i.e. a cut group)
        to be added to a CutflowProcessor

        Args:
            cp (CutflowProcessor): _description_
            steer (dict): _description_
        """

        super().__init__(cp, steer)

        if not cut_group in steer['cuts']:
            raise Exception(f'No cut group with name <{cut_group}> registered. Create an entry at steer["cuts"] with key "{cut_group}" and value None.')

        self._cut_group = cut_group
        self._step_out = step
    
    def attachCutGroup(self):
        """Attaches the cuts of this processor to the CutflowProcessor
        Must be called once the cuts have been inferred in an implementing class
        """

        self._cp._cuts[self._step_out] = self.fetchCutGroup()

    @abstractmethod
    def fetchCutGroup(self)->list[ValueCut]:
        """Method to be implemented by an implementing action. Should return a list
        of ValueCut items to be evaluated.

        Raises:
            NotImplementedError: _description_

        Returns:
            float: _description_
        """
        raise NotImplementedError('fetchCutGroup must be implemented')