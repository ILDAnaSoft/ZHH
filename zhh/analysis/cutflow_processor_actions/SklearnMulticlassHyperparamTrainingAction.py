from math import ceil
from multiprocessing import cpu_count, Pool
from copy import deepcopy
from typing import TYPE_CHECKING
import os, pickle, shutil

from ..CutflowProcessorAction import CutflowProcessorAction, CutflowProcessor, FileBasedProcessorAction
from .MVAThresholdFinderInterface import MVAThresholdFinderInterface
from .SklearnMulticlassTrainingAction import MVATrainingConfig, objective, OptunaSuggestion, parse_suggestions
from .mva_tools import get_signal_categories, get_background_categories

if TYPE_CHECKING:
    import optuna

def objective_mod(trial:'optuna.Trial'):
    from zhh.analysis.cutflow_processor_actions.SklearnMulticlassTrainingAction import configs
    cfg = configs[trial.study.study_name]
     
    hyperparams = {
        **deepcopy(cfg._hyperparams),
        **parse_suggestions(trial, cfg._hyperparam_bounds) }

    return objective(cfg, hyperparams, trial=trial)

def run_optimization(cfg:MVATrainingConfig):
    import optuna
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

    study.optimize(objective_mod, n_trials=cfg._n_trials, n_jobs=1)

class SklearnMulticlassHyperparamTrainingAction(MVAThresholdFinderInterface, FileBasedProcessorAction):
    def __init__(self, cp:CutflowProcessor, steer:dict, mva:str, hyperparam_bounds:list[OptunaSuggestion],
                 clf_prop:str='clf', trial_name:str|None=None, ntrials:int=500, nprocesses:int|None=None,
                 hyperparams:dict={}, assume_singly_study:bool=True, **kwargs):
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
            hyperparams (dict, optional): Optional hyperparameters used as base for final set
                                          I.e. parameters from hyperparam_bounds are merged
                                          into a copy of this.
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
        self._cfg = MVATrainingConfig(os.getcwd(), ntrials,
                                                        trial_name=f'{os.environ["hypothesis"]}.{self._trial_name}',
                                                        signal_categories=self._signal_categories,
                                                        background_categories=self._background_categories,
                                                        hyperparams=hyperparams,
                                                        hyperparam_bounds=hyperparam_bounds,
                                                        trial_data_file=self._data_file)
        self._assume_singly_study = assume_singly_study
        self._cfg.register()
    
    def output(self):
        return self.localTarget(self._mva_file)

    def run(self):
        NPROCESSES = int(self._nprocesses) if self._nprocesses is not None else ceil(.5 * cpu_count())

        print(f'Sig categories', self._signal_categories)
        print(f'Bkg categories', self._background_categories)

        if not os.path.isfile(self._data_file):
            raise Exception(f'Cannot find data file for hyperparameter optimization: {self._data_file}. Did you run WriteMVADataAction before?')

        cfg = self._cfg
        n_left = cfg._n_trials - len(self.getStudy(readonly=True).trials)
        n_workers = min(NPROCESSES, n_left) if n_left > 0 else 0
        cfg._n_trials = ceil(n_left / n_workers) if n_workers > 0 else 0

        print(f'Using {NPROCESSES} cores to sample and try {n_left} hyperparameter sets')

        if n_left > 0:
            with Pool(processes=n_workers) as pool:
                pool.map(run_optimization, [cfg] * n_workers)

        # assign threshold
        if not self.complete():
            raise Exception(f'Unexpected error when training MVA <{self._trial_name}>')
    
    def checkJournalFile(self):

        if self._assume_singly_study:
            # we require all entries in the journal to be bound to study_id" = 0

            with open(self.getJournalPath(), 'rt') as jf:
                text = jf.read()

            for i in range(1, 10):
                needle = f'"study_id":{i}'
                if needle in text:
                    print(f'Warning: There was an invalid study_id={i} found in the log of the hyperparameter optimization. This will be replaced with study_id=0')

                    text = text.replace(needle, '"study_id":0')
            
            with open(self.getJournalPath(), 'wt') as jf:
                jf.write(text)

    def getJournalPath(self):
        return f'{self._cfg._trial_path}/journal.log'

    def getStudy(self, readonly:bool=False)->'optuna.Study':
        import optuna
        from optuna.storages import JournalStorage
        from optuna.storages.journal import JournalFileBackend
        
        if not os.path.isdir(os.path.dirname(self.getJournalPath())):
            os.makedirs(os.path.dirname(self.getJournalPath()))

        if os.path.isfile(self.getJournalPath()):
            self.checkJournalFile()
        else:
            readonly = False

        return optuna.create_study(
            study_name=self._trial_name,
            storage=JournalStorage(JournalFileBackend(file_path=self.getJournalPath())),
            load_if_exists=True
        ) if not readonly else optuna.load_study(
            study_name=self._trial_name,
            storage=JournalStorage(JournalFileBackend(file_path=self.getJournalPath())),
        )
    
    def findBestTrial(self):
        study = self.getStudy(readonly=True)
        return f'{self._cfg._trial_path}/{study.best_trial.number}.pickle'
        
    def complete(self)->bool:
        complete = super().complete()

        if not complete:
            study = self.getStudy(readonly=True)

            if len(study.trials) >= self._ntrials:
                mva_file = os.path.abspath(self._mva_file)
                best_trial = self.findBestTrial()

                os.makedirs(os.path.dirname(mva_file), exist_ok=True)
                shutil.copy(best_trial, mva_file)

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