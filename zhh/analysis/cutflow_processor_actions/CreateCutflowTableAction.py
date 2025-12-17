from ..CutflowProcessorAction import FileBasedProcessorAction, CutflowProcessor
import os.path as osp

class CreateCutflowTableAction(FileBasedProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, file:str, **kwargs):
        assert('cutflow_table' in steer)

        super().__init__(cp, steer)
        
        self._file = file        
        self._step_start = kwargs.get('step_start', 0)
        self._step_end = kwargs.get('step_end', 0)
        self._ignore_categories = self._steer['cutflow_table'].get('ignore_categories', [])
    
    def run(self):
        cutflow_table_items:list[tuple[str, str]] = []
        cutflow_table_is_signal:list[str] = []

        for item in self._steer['cutflow_table']['items']:
            cutflow_table_items.append((item['label'], item['category']))
        
            if item.get('is_signal', False):
                cutflow_table_is_signal.append(item['category'])
        
        self._cp.cutflowTable(cutflow_table_items, filename=self._file,
                              step_start=self._step_start, step_end=self._step_end,
                              signal_categories=cutflow_table_is_signal,
                              ignore_categories=self._ignore_categories)
    
    def output(self):
        return [
            self.localTarget(f'{osp.splitext(self._file)[0]}_efficiency.pdf'),
            self.localTarget(f'{osp.splitext(self._file)[0]}_abs.pdf')
        ]