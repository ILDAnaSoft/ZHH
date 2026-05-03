from ..CutflowProcessorAction import FileBasedProcessorAction, CutflowProcessor
from typing import Sequence
import os.path as osp

class CreateCutflowTableAction(FileBasedProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, file:str, weight_props:list[str], **kwargs):
        """_summary_

        Args:
            cp (CutflowProcessor): _description_
            steer (dict): _description_
            file (str): _description_
            weight_props (list[str]): _description_
        """

        assert('cutflow_table' in steer)

        super().__init__(cp, steer)
        
        self._file = file        
        self._step_start = kwargs.get('step_start', 0)
        self._step_end = kwargs.get('step_end', 0)
        self._weight_props = weight_props
        self._cutflow_table_entries = parse_cutflow_table_entries(steer)
    
    def run(self):
        cutflow_table_items:list[tuple[str, str]] = []
        cutflow_table_is_signal:list[str] = []

        for item in self._steer['cutflow_table']['items']:
            cutflow_table_items.append((item['label'], item['category']))
        
            if item.get('is_signal', False):
                cutflow_table_is_signal.append(item['category'])
        
        self._cp.cutflowTable(self._cutflow_table_entries, filename=self._file,
                              step_start=self._step_start, step_end=self._step_end,
                              weight_props=self._weight_props)
    
    def output(self):
        return [
            self.localTarget(f'{osp.splitext(self._file)[0]}_efficiency.pdf'),
            self.localTarget(f'{osp.splitext(self._file)[0]}_abs.pdf')
        ]
    
def parse_cutflow_table_entries(steer:dict):
    from zhh import CutflowTableEntry, LatexCutflowTableEntry, SumCutflowTableEntry, \
        CategorizedCutflowTableEntry, UncategorizedCutflowTableEntry

    cutflow_table_entries:Sequence[CutflowTableEntry|LatexCutflowTableEntry] = []

    for item in steer['cutflow_table']['items']:
        if 'category' in item:
            entry = CategorizedCutflowTableEntry(**item)
        elif 'remaining_of_source' in item:
            entry = UncategorizedCutflowTableEntry(**item)
        elif 'latex' in item:
            entry = LatexCutflowTableEntry(item['latex'])
        elif 'sum' in item:
            entry = SumCutflowTableEntry(**item)
        else:
            print(item)
            raise Exception('Cannot parse to CutflowTableEntries')

        cutflow_table_entries += [entry]
    
    return cutflow_table_entries