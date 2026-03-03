from math import ceil
from multiprocessing import cpu_count, Pool
import os, pickle, shutil

from ..CutflowProcessorAction import CutflowProcessorAction, CutflowProcessor, FileBasedProcessorAction
from .MVAThresholdFinderInterface import MVAThresholdFinderInterface
from .SklearnMulticlassTrainingAction import MVATrainingConfig, objective, OptunaSuggestion, parse_suggestions
from .mva_tools import get_signal_categories, get_background_categories
import optuna

def objective_mod(trial:optuna.Trial):
    from zhh.analysis.cutflow_processor_actions.SklearnMulticlassTrainingAction import configs
    cfg = configs[trial.study.study_name]
    
    hyper_params = parse_suggestions(trial, cfg._hyperparam_bounds)
    return objective(cfg, hyper_params, cfg._signal_categories, cfg._background_categories, trial=trial)

def run_optimization(cfg:MVATrainingConfig):
    from optuna.storages import JournalStorage
    from optuna.storages.journal import JournalFileBackend
    
    journal_path = f'{cfg._trial_path}/journal.log'

    if not os.path.isdir(os.path.dirname(journal_path)):
        os.makedirs(os.path.dirname(journal_path))

    study = optuna.create_study(
        study_name=cfg._trial_name,
        storage=JournalStorage(JournalFileBackend(file_path=journal_path)),
        load_if_exists=True
    )

    study.optimize(objective_mod, n_trials=cfg._n_trials)

class SklearnMulticlassHyperparamTrainingAction(MVAThresholdFinderInterface, FileBasedProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, mva:str, hyperparam_bounds:list[OptunaSuggestion],
                 clf_prop:str='clf', trial_name:str|None=None, ntrials:int=500, nprocesses:int|None=None, **kwargs):
        """_summary_

        Args:
            cp (CutflowProcessor): _description_
            steer (dict): _description_
            mva (str): _description_
            hyperparam_bounds (list[OptunaSuggestion]): _description_
            clf_prop (str, optional): _description_. Defaults to 'clf'.
            trial_name (str | None, optional): _description_. Defaults to None.
            ntrials (int, optional): Number of hyperparameter sets to try. Defaults to 500.
            nprocesses (int, optional): Number of processes/CPU cores to use. If None, will
                                        use 80%. Defaults to None.
        """
        assert('mvas' in steer)

        super().__init__(cp, steer, mva=mva)

        from zhh import find_by

        mva_spec = find_by(steer['mvas'], 'name', mva, is_dict=True)
        
        self._data_file = mva_spec['data_file']
        self._mva_file = mva_spec['mva_file']
        self._ntrials = ntrials
        self._clf_prop = clf_prop
        self._trial_name = trial_name if trial_name is not None else mva
        self._hyperparam_bounds = hyperparam_bounds
        self._nprocesses = nprocesses

        self._signal_categories = get_signal_categories(steer['signal_categories'], mva_spec['classes'])
        self._background_categories = get_background_categories(self._signal_categories, mva_spec['classes'])

        self._features = mva_spec['features']
        self._cfg:MVATrainingConfig = MVATrainingConfig(os.getcwd(), ntrials, trial_name=f'{os.environ["hypothesis"]}.{self._trial_name}',
                                                        signal_categories=self._signal_categories,
                                                        background_categories=self._background_categories,
                                                        hyperparam_bounds=hyperparam_bounds,
                                                        trial_data_file=self._data_file)
        self._cfg.register()
    
    def output(self):
        return self.localTarget(self._mva_file)

    def run(self):
        NPROCESSES = int(self._nprocesses) if self._nprocesses is not None else ceil(.8 * cpu_count())

        print(f'Sig categories', self._signal_categories)
        print(f'Bkg categories', self._background_categories)

        cfg = self._cfg
        n_left = cfg._n_trials - len(self.getStudy().trials)

        print(f'Using {NPROCESSES} cores to sample and try {n_left} hyperparameter sets')

        if n_left > 0:
            with Pool(processes=NPROCESSES) as pool:
                pool.map(run_optimization, [cfg] * n_left) #range(self._ntrials * NPROCESSES))

        if not self.complete():
            raise Exception(f'Unexpected error when training MVA <{self._trial_name}>')

    def getStudy(self)->optuna.Study:
        import optuna
        from optuna.storages import JournalStorage
        from optuna.storages.journal import JournalFileBackend
        
        journal_path = f'{self._cfg._trial_path}/journal.log'

        if not os.path.isdir(os.path.dirname(journal_path)):
            os.makedirs(os.path.dirname(journal_path))

        return optuna.create_study(
            study_name=self._trial_name,
            storage=JournalStorage(JournalFileBackend(file_path=journal_path)),
            load_if_exists=True
        )
    
    def findBestTrial(self):
        study = self.getStudy()
        return f'{self._cfg._trial_path}/{study.best_trial.number}.pickle'
        
    def complete(self)->bool:
        complete = super().complete()

        if not complete:
            study = self.getStudy()
            
            if len(study.trials) >= self._ntrials:
                mva_file = os.path.abspath(self._mva_file)
                best_trial = self.findBestTrial()

                os.makedirs(os.path.dirname(mva_file), exist_ok=True)
                shutil.copy(best_trial, mva_file)

                print(f'Found best trial at {best_trial}')
                complete = True
            else:
                return False                    
        
        if complete:
            self.assignThreshold(self.findThreshold())
        
        return complete
    
    def findThreshold(self) -> float:
        with open(self._mva_file, 'rb') as pf:
            dump = pickle.load(pf)
        
        return dump['thresh']