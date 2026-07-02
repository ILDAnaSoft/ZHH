from io import StringIO
from copy import deepcopy
from contextlib import redirect_stdout
from typing import TypedDict, Required, Literal, cast, Any, TYPE_CHECKING
from datetime import datetime
from math import sqrt
from multiprocessing import cpu_count
import os, pickle, sys, subprocess

from ..CutflowProcessorAction import FileBasedProcessorAction, CutflowProcessor
from .MVAThresholdFinderInterface import MVAThresholdFinderInterface
from .mva_tools import get_signal_categories, get_background_categories
import numpy as np

if TYPE_CHECKING:
    import optuna

class SklearnMulticlassTrainingAction(MVAThresholdFinderInterface, FileBasedProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, mva:str, hyperparams:dict,
                 clf_prop:str='clf', trial_name:str|None=None, debug:bool=False,
                 check_hash:bool=True, **kwargs):
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
        self._hyperparams = hyperparams
        self._clf_prop = clf_prop
        self._trial_name = trial_name if trial_name is not None else mva
        self._model = mva_spec.get('model', 'XGBClassifier')

        mva_spec['model'] = self._model

        if not self._model in ['LGBMClassifier', 'XGBClassifier']:
            raise Exception('SklearnMulticlassTrainingAction only supports model=LGBMClassifier or XGBClassifier')

        self._signal_categories = get_signal_categories(steer['signal_categories'], mva_spec['classes'])
        self._background_categories = get_background_categories(self._signal_categories, mva_spec['classes'])
        self._debug = debug
        self._check_hash = check_hash

        self._features = mva_spec['features']
    
    def output(self):
        return self.localTarget(self._mva_file)
    
    def getConfig(self)->'MVATrainingConfig':
        return MVATrainingConfig(os.getcwd(), 1, self._trial_name,
                                 hyperparams=self._hyperparams, # required to make sure that injecting n_jobs to hyperparams will not alter the dict and thereby the hash in complete()
                                 model=self._model,
                                 signal_categories=self._signal_categories,
                                 background_categories=self._background_categories,
                                 clf_file=self._mva_file, clf_prop=self._clf_prop)

    def run(self):
        print(f'Sig categories', self._signal_categories)
        print(f'Bkg categories', self._background_categories)

        qty = objective(self.getConfig(), deepcopy(self._hyperparams), train_test_npz=self._data_file, debug=self._debug)

        # assign threshold
        if not self.complete():
            raise Exception(f'Unexpected error when training MVA <{self._trial_name}>')
        
    def complete(self)->bool:
        file_exist = self.output().exists()
        complete = False
        
        if file_exist:
            # check that the used mva config is the same as the current one
            with open(self._mva_file, 'rb') as pf:
                dump = pickle.load(pf)

            complete = not self._check_hash or ('hash' in dump and dump['hash'] == self.getConfig().hash())

            if not complete and self._debug:
                answer = ''

                while answer.lower() not in ['y', 'n']:
                    answer = input(f'An model was found at {self._mva_file}, but it\'s config does not fit the current settings. Do you want to continue and use the existing MVA? If not, the current file will be overwritten (y/n)')

                complete = answer.lower() == 'y'

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
configs:dict[str, 'MVATrainingConfig'] = {}

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
                 signal_categories:list[int]=[], background_categories:list[int]=[],
                 hyperparams: dict[str, Any]={},
                 hyperparam_bounds:list[OptunaSuggestion]=[],
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
        self._hyperparams = hyperparams
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

def parse_suggestions(trial:'optuna.Trial', suggestions:list[OptunaSuggestion])->dict:
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

def objective(config:MVATrainingConfig, hyper_params:dict, trial=None,
              train_test_npz:str|None=None, n_trial:int=0, debug:bool=False, hash:str|None=None):

    import os.path as osp, os
    from zhh import Tee

    trial:optuna.Trial|None

    signal_classes = config._signal_categories
    background_classes = config._background_categories
    clf_property = config._clf_prop
    clf_file = config._clf_file

    if hash is None:
        hash = config.hash()

    if trial is not None:
        n_trial = trial.number
        hyper_params['n_jobs'] = 1
        print(f'Running trial {n_trial} in process {os.getpid()}')

    if clf_file is None:
        clf_file = f'{config._trial_path}/{n_trial}.pickle'

    os.makedirs(osp.dirname(osp.abspath(clf_file)), exist_ok=True)
    
    # expects train_test_npz to be in a format as written by WriteMVADataAction
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

    is_signal = np.isin(y_test, signal_classes)
    is_background = np.isin(y_test, background_classes)

    y_test_pred = cast(np.ndarray, clf.predict_proba(X_test))
    y_train_pred = cast(np.ndarray, clf.predict_proba(X_train))

    if len(signal_classes) <= 1:
        NITEMS = 100

        x = np.linspace(0.5, 0.99, NITEMS, endpoint=False)
        sig = np.zeros(NITEMS)
        bkg = np.zeros(NITEMS)

        for i, t in enumerate(np.nditer(x)):
            thresh = t

            sig[i] = w_test_phys[is_signal     & (y_test_pred[:, signal_classes].sum(axis=1) >= thresh)].sum()
            bkg[i] = w_test_phys[is_background & (y_test_pred[:, signal_classes].sum(axis=1) >= thresh)].sum()

        significances = sig/np.sqrt(sig + bkg)
        significances = np.nan_to_num(significances)
        best_significance = np.max(significances)

        max_pos = np.argmax(significances)

        # show stats in train dataset
        thresh = x[max_pos]
        sig_train = w_train_phys[np.isin(y_train, signal_classes    ) & (y_train_pred[:, signal_classes].sum(axis=1) >= thresh)].sum()
        bkg_train = w_train_phys[np.isin(y_train, background_classes) & (y_train_pred[:, signal_classes].sum(axis=1) >= thresh)].sum()
        significance_train = sig_train/sqrt(sig_train + bkg_train)

        print(f'trial {n_trial}: test dataset : nsig={sig[max_pos]:3f} nbkg={bkg[max_pos]:.3f} (sign={best_significance:.3f})')
        print(f'trial {n_trial}: train dataset: nsig={sig_train:3f} nbkg={bkg_train:.3f} (sign={significance_train:.3f})')
    else:
        # multiple signal classes: rather than cutting on the summed MVA output of all
        # signal classes with a single threshold, optimize 2 cut parameters per signal
        # class (a lower bound on its own score, an upper bound on the summed score of
        # the other signal classes) and combine the per-class significances in quadrature
        from scipy.optimize import minimize

        def make_selections(probas:np.ndarray, params) -> list[np.ndarray]:
            selections = []

            for i, cls in enumerate(signal_classes):
                own = probas[:, cls]
                others = probas[:, [c for c in signal_classes if c != cls]].sum(axis=1)
                selections.append((own >= params[2*i]) & (others <= params[2*i + 1]))

            return selections

        def combined_significance(probas:np.ndarray, weights:np.ndarray, params, sig_mask:np.ndarray, bkg_mask:np.ndarray):
            sigs = np.zeros(len(signal_classes))
            bkgs = np.zeros(len(signal_classes))

            for i, selection in enumerate(make_selections(probas, params)):
                sigs[i] = weights[selection & sig_mask].sum()
                bkgs[i] = weights[selection & bkg_mask].sum()

            significances = np.nan_to_num(sigs/np.sqrt(sigs + bkgs))
            return sigs, bkgs, sqrt((significances ** 2).sum()), significances

        def objective_multiclass(params):
            _, _, significance, _ = combined_significance(y_test_pred, w_test_phys, params, is_signal, is_background)
            return 1./significance if significance > 0 else np.inf

        bounds = [(0.5, 1.), (0., 1.)] * len(signal_classes)
        x0 = [.9, .1] * len(signal_classes)

        res = minimize(objective_multiclass, x0=x0, bounds=bounds, method='Nelder-Mead')

        thresh = res.x
        x = None
        sig, bkg, best_significance, sub_significances = combined_significance(y_test_pred, w_test_phys, thresh, is_signal, is_background)

        is_signal_train = np.isin(y_train, signal_classes)
        is_background_train = np.isin(y_train, background_classes)
        sig_train_arr, bkg_train_arr, significance_train, sub_significances_train = combined_significance(y_train_pred, w_train_phys, thresh, is_signal_train, is_background_train)
        sig_train = sig_train_arr.sum()
        bkg_train = bkg_train_arr.sum()

        print(f'trial {n_trial}: test dataset : nsig={sig.sum():3f} nbkg={bkg.sum():.3f} (sign: comb={best_significance:.3f}, ' + \
              ', '.join([ f'cls {label}: {sign:.3f}' for label, sign in zip(signal_classes, sub_significances) ]) + ')' )
        print(f'trial {n_trial}: train dataset: nsig={sig_train:3f} nbkg={bkg_train:.3f} (sign: comb={significance_train:.3f}, ' + \
              ', '.join([ f'cls {label}: {sign:.3f}' for label, sign in zip(signal_classes, sub_significances_train) ]) + ')' )

    dump = {
        'hyper_params': hyper_params,
        'loss_history': loss_history,
        'sig': sig,
        'bkg': bkg,
        'x': x,
        'features': data['features'],
        'classes': data['classes'],
        'train_sample_size': X_train.shape[0],
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