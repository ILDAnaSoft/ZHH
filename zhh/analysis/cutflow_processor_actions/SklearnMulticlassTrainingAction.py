from io import StringIO
from contextlib import redirect_stdout
from typing import TypedDict, Required, Literal, cast
from datetime import datetime
from math import sqrt
from multiprocessing import cpu_count
import os, pickle, sys, subprocess

from ..CutflowProcessorAction import FileBasedProcessorAction, CutflowProcessor
from .MVAThresholdFinderInterface import MVAThresholdFinderInterface
from .mva_tools import get_signal_categories, get_background_categories
import numpy as np
import optuna

class SklearnMulticlassTrainingAction(MVAThresholdFinderInterface, FileBasedProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, mva:str, hyperparams:dict,
                 clf_prop:str='clf', trial_name:str|None=None, debug:bool=False, **kwargs):
        """_summary_

        Args:
            cp (CutflowProcessor): _description_
            steer (dict): _description_
            hyperparams (dict): MVA hyperparameters
            mva (str): _description_
            clf_prop (str, optional): _description_. Defaults to 'clf'.
            trial_name (str|None, optional): Name of the hyperparameter search trial for Optuna.
                                             will be replaced with mva if None. Defaults to None.
            
        """
        assert('mvas' in steer)

        super().__init__(cp, steer, mva=mva)

        from zhh import find_by

        self._mva_spec = mva_spec = find_by(steer['mvas'], 'name', mva, is_dict=True)

        self._data_file = mva_spec['data_file']
        self._mva_file = mva_spec['mva_file']
        self._clf_prop = clf_prop
        self._trial_name = trial_name if trial_name is not None else mva
        self._model = mva_spec.get('model', 'XGBClassifier')

        mva_spec['model'] = self._model

        if not self._model in ['LGBMClassifier', 'XGBClassifier']:
            raise Exception('SklearnMulticlassTrainingAction only supports model=LGBMClassifier or XGBClassifier')

        self._signal_categories = get_signal_categories(steer['signal_categories'], mva_spec['classes'])
        self._background_categories = get_background_categories(self._signal_categories, mva_spec['classes'])
        self._debug = debug

        self._features = mva_spec['features']
        self._hyperparams = hyperparams
    
    def output(self):
        return self.localTarget(self._mva_file)
    
    def getConfig(self)->'MVATrainingConfig':
        return MVATrainingConfig(os.getcwd(), 1, self._trial_name, model=self._model,
                                 signal_categories=self._signal_categories,
                                 background_categories=self._background_categories,
                                 clf_file=self._mva_file, clf_prop=self._clf_prop)

    def run(self):
        print(f'Sig categories', self._signal_categories)
        print(f'Bkg categories', self._background_categories)

        qty = objective(self.getConfig(), self._hyperparams, train_test_npz=self._data_file, debug=self._debug)
        
    def complete(self)->bool:
        file_exist = super().complete()
        complete = False
        
        if file_exist:
            # check that the used mva config is the same as the current one
            with open(self._mva_file, 'rb') as pf:
                dump = pickle.load(pf)

            complete = 'hash' in dump and dump['hash'] == self.getConfig().hash()

        if complete:
            self.assignThreshold(self.findThreshold())
        
        return complete
    
    def findThreshold(self) -> float:
        with open(self._mva_file, 'rb') as pf:
            dump = pickle.load(pf)
        
        return dump['thresh']

class OptunaSuggestion(TypedDict):
    name: Required[str]
    type: Required[Literal['int']|Literal['float']]
    lower: Required[int|float]
    upper: Required[int|float]

# global registry of MVATrainingConfig
# used only when using multiprocessing, e.g. in SklearnMulticlassHyperparamTrainingAction
configs = {}

def find_subclss(cls, subclss:list=[]):
    if hasattr(cls, '__subclasses__'):
        for subcls in cls.__subclasses__():
            if subcls not in subclss:
                subclss.append(subcls)
            
            find_subclss(subcls, subclss)
    
    return subclss

def parse_to_string(value):
    try:
        string_rep = str(value)
    except Exception as e:
        string_rep = f'Unknown'

    return string_rep

class MVATrainingConfig:
    def __init__(self, base_path:str, n_trials:int,
                 trial_name:str|None, trial_data:str|None=None, trial_data_file:str='train_test.npz',
                 training_mode:Literal['loss']|Literal['significance']='significance',
                 signal_categories:list[int]=[], background_categories:list[int]=[], hyperparam_bounds:list[OptunaSuggestion]=[],
                 model:Literal['LGBMClassifier', 'XGBClassifier']='XGBClassifier',
                 clf_file:str|None=None, clf_prop:str='clf'):
        
        """_summary_

        Args:
            base_path (str): _description_
            n_trials (int): number of trials to train (1 for single run, n>1 for multi-trial)
            trial_name (str | None): _description_
            trial_data (str | None, optional): _description_. Defaults to None.
            trial_data_file (str, optional): _description_. Defaults to 'train_test.npz'.
            training_mode (Literal["loss"] | Literal["significance"], optional): _description_. Defaults to 'significance'.
        """
        if trial_name is None:
            trial_name = datetime.now().strftime('%Y%m%d.%H%M%S')

        assert(training_mode.lower() in ['loss', 'significance'])

        self._trial_data = f'{base_path}/{trial_data_file}' if trial_data is None else trial_data
        self._n_trials = n_trials
        self._clf_file = clf_file
        self._trial_name = trial_name
        self._base_path  = base_path
        self._trial_path = f'{base_path}/trial-{trial_name}'
        self._training_mode = training_mode.lower()
        self._signal_categories = signal_categories
        self._background_categories = background_categories
        self._hyperparam_bounds = hyperparam_bounds
        self._model = model
        self._clf_prop = clf_prop
    
    def __str__(self)->str:
        from json import dumps
        dump = ''
        for key, val in self.__dict__.items():
            dump += f'\n{key}={parse_to_string(val)}'

        return f'<MVATrainingConfig {dump}>'

    def hash(self)->str:
        from hashlib import md5
        return md5(self.__str__().encode()).hexdigest()

    def register(self):
        if self._trial_name in configs:
            raise Exception(f'MVA trial <{self._trial_name}> already registered!')

        configs[self._trial_name] = self
    
    def release(self):
        del configs[self._trial_name]

def parse_suggestions(trial:optuna.Trial, suggestions:list[OptunaSuggestion])->dict:
    """Given an optuna trial and a list of hyperparameter bounds,
    samples a set of hyperparameters and returns it as dictionary.

    Args:
        trial (optuna.Trial): _description_
        suggestions (list[OptunaSuggestion]): _description_

    Returns:
        dict: _description_
    """
    kwargs = {}
    
    for suggestion in suggestions:
        assert(suggestion['type'] == 'int' or suggestion['type'] == 'float')        
        kwargs[suggestion['name']] = getattr(trial, 'suggest_int' if suggestion['type'] == 'int' else 'suggest_float')(
            suggestion['name'], suggestion['lower'], suggestion['upper'])
        
    return kwargs

def objective(config:MVATrainingConfig, hyper_params:dict, trial:optuna.Trial|None=None,
              train_test_npz:str|None=None, n_trial:int=0, debug:bool=False, hash:str|None=None):
    
    import os.path as osp, os
    from zhh import Tee

    signal_classes = config._signal_categories
    background_classes = config._background_categories
    clf_property = config._clf_prop
    clf_file = config._clf_file

    if hash is None:
        hash = config.hash()

    if trial is not None:
        n_trial = trial.number
        print(f'Running trial {n_trial} in process {os.getpid()}')

    if clf_file is None:
        clf_file = f'{config._trial_path}/{n_trial}.pickle'

    os.makedirs(osp.dirname(osp.abspath(clf_file)), exist_ok=True)
    
    if train_test_npz is None:
        train_test_npz = config._trial_data
    
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

        if not 'n_jobs' in hyper_params:
            hyper_params['n_jobs'] = round(0.8*cpu_count())

        if config._model == 'XGBClassifier':
            from xgboost import XGBClassifier

            clf = XGBClassifier(**hyper_params)
            clf.fit(X_train, np.array(y_train, dtype=int),
                    eval_set=[(X_test, np.array(y_test, dtype=int))],
                    verbose=True, sample_weight=w_train)
        elif config._model == 'LGBMClassifier':
            from lightgbm import LGBMClassifier, log_evaluation

            clf = LGBMClassifier(**hyper_params)
            clf.fit(X_train, np.array(y_train, dtype=int),
                    eval_set=[(X_test, np.array(y_test, dtype=int))],
                    callbacks=[log_evaluation(period=1)], sample_weight=w_train)
        else:
            raise Exception(f'Unknown model <{config._model}>')
    finally:
        sys.stdout = old_stdout

    # following works for XGB and LightGBM
    loss_history = list(list(clf.evals_result_.values())[0].values())[0]

    NITEMS = 500

    x = np.linspace(0, 1, NITEMS, endpoint=False)
    sig = np.zeros(NITEMS)
    bkg = np.zeros(NITEMS)

    is_signal = np.isin(y_test, signal_classes)
    is_background = np.isin(y_test, background_classes)

    y_test_pred = cast(np.ndarray, clf.predict_proba(X_test))

    for i, t in enumerate(np.nditer(x)):
        thresh = t

        sig[i] = w_test_phys[is_signal     & (y_test_pred[:, signal_classes].sum(axis=1) >= thresh)].sum()
        bkg[i] = w_test_phys[is_background & (y_test_pred[:, signal_classes].sum(axis=1) >= thresh)].sum()
    
    significances = sig/np.sqrt(sig + bkg)
    significances = np.nan_to_num(significances)
    best_significance = np.max(significances)
    
    max_pos = np.argmax(significances)
    
    # show stats in train dataset
    y_train_pred = cast(np.ndarray, clf.predict_proba(X_train))
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
        'thresh': thresh,
        'hash': hash
    }
    dump[clf_property] = clf

    with open(clf_file, 'wb') as pf:
        pickle.dump(dump, pf)
    
    if config._training_mode == 'loss':
        return loss_history[-1] if len(loss_history) else []
    elif config._training_mode == 'significance':
        return 1./float(best_significance)
    else:
        raise Exception(f'Unknown training mode <{config._training_mode}>')