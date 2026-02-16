from ..CutflowProcessorAction import FileBasedProcessorAction, CutflowProcessor
from .MVAThresholdFinderInterface import MVAThresholdFinderInterface
from io import StringIO
from xgboost import XGBClassifier
from datetime import datetime
from math import sqrt
from typing import TypedDict, Required, Literal
import os, pickle, sys, subprocess
import optuna
import numpy as np
from .mva_tools import get_signal_categories, get_background_categories
from contextlib import redirect_stdout

class SklearnMulticlassTrainingAction(MVAThresholdFinderInterface, FileBasedProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, mva:str, hyperparams:dict,
                 clf_prop:str='clf', trial_name:str='default', debug:bool=False, **kwargs):
        """_summary_

        Args:
            cp (CutflowProcessor): _description_
            steer (dict): _description_
            hyperparams (dict): MVA hyperparameters
            mva (str): _description_
            clf_prop (str, optional): _description_. Defaults to 'clf'.
            trial_name (str, optional): Name of the hyperparameter search trial for Optuna. Defaults to 'default'.
            
        """
        assert('mvas' in steer)

        super().__init__(cp, steer, mva=mva)

        from zhh import find_by

        mva_spec = find_by(steer['mvas'], 'name', mva, is_dict=True)
        
        self._data_file = mva_spec['data_file']
        self._mva_file = mva_spec['mva_file']
        self._clf_prop = clf_prop
        self._trial_name = trial_name

        self._signal_categories = get_signal_categories(steer['signal_categories'], mva_spec['classes'])
        self._background_categories = get_background_categories(self._signal_categories, mva_spec['classes'])
        self._debug = debug

        self._features = mva_spec['features']
        self._hyperparams = hyperparams
    
    def output(self):
        return self.localTarget(self._mva_file)

    def run(self):
        from zhh import DataExtractor

        print(f'Sig categories', self._signal_categories)
        print(f'Bkg categories', self._background_categories)

        set_conf(os.getcwd(), self._trial_name)

        qty = objective(self._hyperparams, self._signal_categories, self._background_categories,
                        clf_file=self._mva_file, clf_property=self._clf_prop, train_test_npz=self._data_file,
                        debug=self._debug)
        
    def complete(self)->bool:
        complete = super().complete()
        if complete:
            self.assignThreshold(self.findThreshold())
        
        return complete
    
    def findThreshold(self) -> float:
        with open(self._mva_file, 'rb') as pf:
            dump = pickle.load(pf)
        
        return dump['thresh']

conf = {
    'DATESTRING': '',
    'BPATH': '',
    'TRIALPATH': ''
}

def set_conf(base_path:str, datestring:str|None=None):
    if datestring is None:
        datestring = datetime.now().strftime('%Y%m%d.%H%M%S')

    conf['DATESTRING'] = datestring
    conf['BPATH'] = base_path
    conf['TRIALPATH'] = f'{base_path}/trial-{datestring}'

class OptunaSuggestion(TypedDict):
    name: Required[str]
    type: Required[Literal['int']|Literal['float']]
    lower: Required[int|float]
    upper: Required[int|float]

def parse_suggestions(trial:optuna.Trial, suggestions:list[OptunaSuggestion])->dict:
    kwargs = {}
    
    for suggestion in suggestions:
        assert(suggestion['type'] == 'int' or suggestion['type'] == 'float')        
        kwargs[suggestion['name']] = getattr(trial, 'suggest_int' if suggestion['type'] == 'int' else 'suggest_float')(
            suggestion['name'], suggestion['lower'], suggestion['upper'])
        
    return kwargs

def objective(hyper_params:dict, signal_classes:list[int], background_classes:list[int], trial:optuna.Trial|None=None,
              clf_file:str|None=None, clf_property:str='clf', train_test_npz:str|None=None, n_trial:int=0, debug:bool=False):
    
    from zhh import Tee

    if trial is not None:
        n_trial = trial.number
        print(f"Running trial {n_trial} in process {os.getpid()}")

    if clf_file is None:
        clf_file = f'{conf["TRIALPATH"]}/{n_trial}.pickle'
    
    if train_test_npz is None:
        train_test_npz = f'{conf["BPATH"]}/train_test.npz'
    
    data = np.load(train_test_npz)

    y_train = data['y_train']
    X_train = data['X_train']
    w_train = data['w_train']
    w_train_phys = data['w_train_phys']

    #if 'sig_wt_mod' in hyper_params:
    #    w_train[y_train == 0] = w_train[y_train == 0] * hyper_params['sig_wt_mod']
    #    del hyper_params['sig_wt_mod']

    y_test = data['y_test']
    X_test = data['X_test']
    w_test_phys = data['w_test_phys']

    # execute
    output_buffer = StringIO()
    old_stdout = sys.stdout
    
    try:
        sys.stdout = Tee(sys.stdout, output_buffer) if debug else output_buffer

        xgbclf = XGBClassifier(**hyper_params)
        xgbclf.fit(X_train, np.array(y_train, dtype=int),
                   eval_set=[(X_test, np.array(y_test, dtype=int))],
                   verbose=True, sample_weight=w_train)
    finally:
        sys.stdout = old_stdout

    loss_history = []

    for line in output_buffer.getvalue().split('\n'):
        if ':' in line:
            loss_history.append(float(line.split(':')[1]))

    NITEMS = 500

    x = np.linspace(0, 1, NITEMS, endpoint=False)
    sig = np.zeros(NITEMS)
    bkg = np.zeros(NITEMS)

    is_signal = np.isin(y_test, signal_classes)
    is_background = np.isin(y_test, background_classes)

    y_test_pred = xgbclf.predict_proba(X_test)

    for i, t in enumerate(np.nditer(x)):
        thresh = t

        sig[i] = w_test_phys[is_signal     & (y_test_pred[:, signal_classes].sum(axis=1) >= thresh)].sum()
        bkg[i] = w_test_phys[is_background & (y_test_pred[:, signal_classes].sum(axis=1) >= thresh)].sum()
    
    significances = sig/np.sqrt(sig + bkg)
    significances = np.nan_to_num(significances)
    best_significance = np.max(significances)
    
    max_pos = np.argmax(significances)
    
    # show stats in train dataset
    y_train_pred = xgbclf.predict_proba(X_train)
    thresh = x[max_pos]
    sig_train = w_train_phys[np.isin(y_train, signal_classes    ) & (y_train_pred[:, signal_classes].sum(axis=1) >= thresh)].sum()
    bkg_train = w_train_phys[np.isin(y_train, background_classes) & (y_train_pred[:, signal_classes].sum(axis=1) >= thresh)].sum()
    significance_train = sig_train/sqrt(sig_train + bkg_train)

    print(f'trial {n_trial}: test dataset : nsig={sig[max_pos]:3f} nbkg={bkg[max_pos]:.3f} (sign={best_significance:.3f})')
    print(f'trial {n_trial}: train dataset: nsig={sig_train:3f} nbkg={bkg_train:.3f} (sign={significance_train:.3f})')
    
    dump = {
        'hyper_params': hyper_params,
        'loss_history': loss_history,
        'sig': sig,
        'bkg': bkg,
        'x': x,
        'best_significance': best_significance,
        'significance_train': significance_train,
        'thresh': thresh
    }
    dump[clf_property] = xgbclf

    with open(clf_file, 'wb') as pf:
        pickle.dump(dump, pf)

    #return xgbclf
    #return loss_history[-1]
    return float(best_significance)**(-1)
    