# Documentation of CutflowProcessorActions

The cutflow tool is used for statistical analysis on large-scale, ROOT-based analysis. However, on it's own, it only manages data IO (i.e. input of feature data in ROOT TTrees), event categorization and weighting. Anything beyond that must be implemented as CutflowProcessorAction. These are classes implementing desired functionalities such as plotting, MVA analysis etc.

## Base classes

Any action must implement at least `CutflowProcessorAction`. If you want to implement a custom action, make sure to import it. Only available subclasses of `CutflowProcessorAction` (or any action inheriting from it) can be used in cutflow.

### CutflowProcessorAction

Each action must implement a `run()` method that does the actual job, a `complete()` method that returns whether or not the action has completed it's job (e.g. by checking whether an output file exists, a dataset exists in each DataStore etc.), and, optionally, a `reset()` method that cleans up the work by `run()`.

### FileBasedProcessorAction

If an action is supposed to create a file output, the `FileBasedProcessorAction` can be used. It requires implementing an `output()` method that should return either one `LocalTarget` or a list of `LocalTargets`, similar to a law task.

## Implementations

TODO

### ApplyCuts

TODO

### CreateCutflowPlotsAction

TODO

### CreateCutflowTableAction

TODO

### SplitDatasets

TODO

### WriteMVAData

TODO

### PrintSplitWeights

TODO

### AttachMatrixElements

TODO

### SklearnMulticlassTraining

TODO

### SklearnMulticlassHyperparamTraining

Similar to `SklearnMulticlassTraining`, but uses a Bayesian optimized hyperparameter search implemented in [Optuna](https://github.com/optuna/optuna) to find optimal hyperparameters. 

Required arguments:

- (str) `mva`: name of the MVA registered in cutflow
- (list[dict]) `hyperparam_bounds`: list of dicts with keys `name`, `type` $\in$ [`int`, `float`], `lower` and `upper` to setup a grid of hyperparameters
- (str) `clf_prop`: key at which the classifier is saved in the output pickle file

Optional arguments:
- (str|None) `trial_name`: Defines the name of the directory in which all trials should be saved as `trial-<trial_name>`. If None, this defaults to the value of `mva`. Defaults to None. 
- (int) `ntrials`: Number of sets of hyperparameters to try. Defaults to 500.
- (int|None) `nprocesses`: Number of processes/CPU cores to use. If None, will use 80%. Defaults to None.

This action will store one pickle file for each set of hyperparameters in the trial directory, along one journal file which includes summary information. The journal `.log` file can be inspected using the [Optuna dashboard](https://optuna.github.io/optuna-dashboard/). The best classifier will be saved at the location given in the MVA definition of cutflow.

### SklearnMulticlassInference

TODO