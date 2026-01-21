from io import StringIO
from xgboost import XGBClassifier
from optuna.storages import JournalStorage
from optuna.storages.journal import JournalFileBackend
from multiprocessing import Pool, cpu_count
from datetime import datetime
from math import sqrt, ceil
import os, pickle, sys, argparse
import optuna
import numpy as np

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

set_conf(os.getcwd())

def objective(trial:optuna.Trial, hyper_params:dict|None=None,
              signal_classes:list[int]=[0, 1], background_classes:list[int]=[2, 3, 4, 5]):
    
    n_trial = trial.number
    print(f"Running trial {n_trial} in process {os.getpid()}")
    
    data = np.load(f'{conf["BPATH"]}/train_test.npz')

    if hyper_params is None:
        hyper_params = {
            'max_depth': trial.suggest_int('max_depth', 3, 7),
            'n_estimators': trial.suggest_int('n_estimators', 250, 750),
            'learning_rate': trial.suggest_float('learning_rate', 0.01, 0.6),
            #'sig_wt_mod': trial.suggest_float('sig_wt_mod', 1, 10)
        }

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
    
    old_stdout = sys.stdout
    try:
        sys.stdout = mystdout = StringIO()

        xgbclf = XGBClassifier(**hyper_params)
        xgbclf.fit(X_train, np.array(y_train, dtype=int),
                   eval_set=[(X_test, np.array(y_test, dtype=int))],
                   verbose=True, sample_weight=w_train)
    finally:
        sys.stdout = old_stdout

    loss_history = []
    if True:
        for line in mystdout.getvalue().split('\n'):
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
    
    pickle.dump({
        'clf': xgbclf,
        'hyper_params': hyper_params,
        'loss_history': loss_history,
        'sig': sig,
        'bkg': bkg,
        'x': x,
        'best_significance': best_significance,
        'significance_train': significance_train,
        'thresh': thresh
    }, open(f'{conf["TRIALPATH"]}/{n_trial}.pickle', 'wb'))

    #return xgbclf
    #return loss_history[-1]
    return float(best_significance)**(-1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-ntrials', type=str, default=500, help='number of trials')
    parser.add_argument('-nprocesses', type=str, default=None, help='number of processes to use. set to 80% of available CPU cores if not given')
    parser.add_argument('-trial', type=str, default=None, help='name/timestamp of trial')
    parser.add_argument('-basepath', type=str, default=None, help='basepath')

    args = parser.parse_args()

    NTRIALS = args.ntrials
    NPROCESSES = ceil(.8 * cpu_count()) if args.nprocesses is None else args.nprocesses
    
    print(f'Using {NPROCESSES} cores')

    # process the steering file -> sources and final state info
    if args.trial is not None:
        set_conf(os.getcwd() if args.basepath is None else args.basepath, args.trial)

    os.makedirs(conf['TRIALPATH'], exist_ok=True)
    
    def run_optimization(_):
        study = optuna.create_study(
            study_name="journal_storage_multiprocess",
            storage=JournalStorage(JournalFileBackend(file_path=f'{conf["TRIALPATH"]}/journal.log')),
            load_if_exists=True, # Useful for multi-process or multi-node optimization.
        )
        study.optimize(objective, n_trials=NTRIALS)

    with Pool(processes=NPROCESSES) as pool:
        pool.map(run_optimization, range(NTRIALS * NPROCESSES))