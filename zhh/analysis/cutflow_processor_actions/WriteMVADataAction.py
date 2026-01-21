from ..CutflowProcessorAction import FileBasedProcessorAction, CutflowProcessor
import numpy as np

class WriteMVADataAction(FileBasedProcessorAction):
    def __init__(self, cp: CutflowProcessor, steer: dict, use:str, split_column:str='split',
                wt_split_column='weights_split', step:int=0, train_split:int=0, test_split:int=1, **kwargs):
        """Writes out MVA data for training and testing. Assumes a splitting into different categories
        has been performed previously using SplitDatasetsAction.

        Args:
            cp (CutflowProcessor): _description_
            steer (dict): _description_
            use (str): name of MVA to use
            split_column (str, optional): _description_. Defaults to 'split'.
            wt_split_column (str, optional): _description_. Defaults to 'weights_split'.
            step (int): n-th cut group of the CutflowProcessor. Defaults to 0.
            train_split (int, optional): _description_. Defaults to 0.
            test_split (int, optional): _description_. Defaults to 1.
        """
        super().__init__(cp, steer)
        
        from zhh import find_by

        mva_spec = find_by(steer['mvas'], 'name', use, is_dict=True)

        self._use = use
        self._classes = mva_spec['classes']
        self._features = mva_spec['features']
        self._data_file = mva_spec['data_file']

        self._step = step
        self._split_column = split_column
        self._wt_split_column = wt_split_column
        self._train_split = train_split
        self._test_split = test_split

    def run(self):
        from zhh import DataExtractor
        extractor = DataExtractor(self._cp)

        src_idx_train, event_num_train, \
        y_train, w_train, w_train_phys, X_train = extractor.extract(self._classes, self._features, step=self._step,
                                                                    split=self._train_split, weight_prop=self._wt_split_column)

        src_idx_test, event_num_test, \
        y_test, w_test, w_test_phys, X_test = extractor.extract(self._classes, self._features, step=self._step,
                                                                split=self._test_split, weight_prop=self._wt_split_column)
        
        np.savez_compressed(self._data_file, features=self._features, classes=self._classes,
                            src_idx_train=src_idx_train, event_num_train=event_num_train, y_train=y_train,
                            w_train=w_train, w_train_phys=w_train_phys, X_train=X_train,
                            src_idx_test=src_idx_test, event_num_test=event_num_test, y_test=y_test,
                            w_test=w_test, w_test_phys=w_test_phys, X_test=X_test)

    def output(self):
        return self.localTarget(self._data_file)