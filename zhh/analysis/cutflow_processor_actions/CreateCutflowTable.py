from ..CutflowProcessorAction import CutflowProcessorAction, CutflowProcessor
import os.path as osp

class CreateCutflowTable(CutflowProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, file:str, **kwargs):
        super().__init__(cp)
        self._file = file
        self._kwargs = kwargs
    
    def run(self):
        cutflow_table_items:list[tuple[str, str]] = []
        cutflow_table_is_signal:list[str] = []

        for item in steer['cutflow_table']['items']:
            cutflow_table_items.append((item['label'], item['category']))

            if item.get('is_signal', False):
                cutflow_table_is_signal.append(item['category'])
            
        self._cp.cutflowTable(cutflow_table_items, filename=self._file,
                              step_start=self._kwargs['step_start'], step_end=self._kwargs['step_end'],
                              signal_categories=cutflow_table_is_signal,
                              ignore_categories=self._kwargs['ignore_categories'])

        self._cp.cutflowTable(display=False, file=self._file, **self._kwargs)
    
    def done(self):
        return osp.isfile(f'{osp.splitext(self._file)[0]}_abs.pdf')