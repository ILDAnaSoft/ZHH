from io import StringIO
from xgboost import XGBClassifier
from optuna.storages import JournalStorage
from optuna.storages.journal import JournalFileBackend
from multiprocessing import Pool, cpu_count
from datetime import datetime
from math import sqrt, ceil
from typing import TypedDict, Required, NotRequired, Literal
import os, pickle, sys, argparse
import optuna
import numpy as np
from zhh.analysis.cutflow_processor_actions.SklearnMulticlassTrainingAction import objective, set_conf, conf

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-ntrials', type=str, default=500, help='number of trials')
    parser.add_argument('-nprocesses', type=str, default=None, help='number of processes to use. set to 80% of available CPU cores if not given')
    parser.add_argument('-trial', type=str, default='default', help='name/timestamp of trial')
    parser.add_argument('-basepath', type=str, default=None, help='basepath')
    parser.add_argument('-signal_classes', type=str, default='0,1', help='comma-separated list of class labels for the signal')
    parser.add_argument('-background_classes', type=str, default='2,3,4,5', help='comma-separated list of class labels for the background')

    args = parser.parse_args()

    NTRIALS = args.ntrials
    NPROCESSES = ceil(.8 * cpu_count()) if args.nprocesses is None else args.nprocesses

    signal_classes:list[int] = [int(i) for i in ','.split(args.signal_classes)]
    background_classes:list[int] = [int(i) for i in ','.split(args.background_classes)]

    def objective_mod(trial):
        hyper_params = {
            'max_depth': trial.suggest_int('max_depth', 3, 7),
            'n_estimators': trial.suggest_int('n_estimators', 250, 750),
            'learning_rate': trial.suggest_float('learning_rate', 0.01, 0.6),
            #'sig_wt_mod': trial.suggest_float('sig_wt_mod', 1, 10)
        }

        return objective(hyper_params, signal_classes, background_classes, trial=trial)
    
    print(f'Using {NPROCESSES} cores')

    # process the steering file -> sources and final state info
    set_conf(os.getcwd() if args.basepath is None else args.basepath, args.trial if args.trial is not None else None)

    os.makedirs(conf['TRIALPATH'], exist_ok=True)
    
    def run_optimization(_):
        study = optuna.create_study(
            study_name="journal_storage_multiprocess",
            storage=JournalStorage(JournalFileBackend(file_path=f'{conf["TRIALPATH"]}/journal.log')),
            load_if_exists=True, # Useful for multi-process or multi-node optimization.
        )
        study.optimize(objective_mod, n_trials=NTRIALS)

    with Pool(processes=NPROCESSES) as pool:
        pool.map(run_optimization, range(NTRIALS * NPROCESSES))