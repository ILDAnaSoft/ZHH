# Documentation of CutflowProcessorActions

The cutflow tool is used for statistical analysis on large-scale, ROOT-based analysis. However, on it's own, it only manages data IO (i.e. input of feature data in ROOT TTrees), event categorization and weighting. Anything beyond that must be implemented as CutflowProcessorAction. These are classes implementing desired functionalities such as plotting, MVA analysis etc.

## Base classes

Any action must implement at least `CutflowProcessorAction`. If you want to implement a custom action, make sure to import it. Only available subclasses of `CutflowProcessorAction` (or any action inheriting from it) can be used in cutflow.

### CutflowProcessorAction

Each action must implement a `run()` method that does the actual job, a `complete()` method that returns whether or not the action has completed it's job (e.g. by checking whether an output file exists, a dataset exists in each DataStore etc.), and, optionally, a `reset()` method that cleans up the work by `run()`.

### FileBasedProcessorAction

If an action is supposed to create a file output, the `FileBasedProcessorAction` can be used. It requires implementing an `output()` method that should return either one `LocalTarget` or a list of `LocalTargets`, similar to a law task.

## Abstract classes

TODO

### MVAThresholdFinderInterface

Allows an MVA optimizer to dynamically supply a threshold value. An action inheriting from this can dynamically bind properties to registered MVAs. E.g., a property threshold can be assigned to a MVA default_mva and referenced in a greater-equal-cut as default_mva.threshold.

Must implement: findThreshold()

## Implementations

TODO

### ApplyCuts

TODO

### CreateCutflowPlotsAction

TODO

### CreateCutflowTableAction

TODO

### SplitDatasets

Splits the dataset in n smaller subsets, e.g. for train/testing. The splitting is done accross all steps, i.e. regardless of cut group.

The action creates two new columns: `split_column` and `wt_split_column`.

1. `uint8` column split_column; for n entries in fractions:
    - value 0 is assigned to a fraction of events specified at position 0 in fractions
    - value `1[..]n-1` to fractions given at position `1[..]n-1`
    - value n is assigned to the remaining events [fraction (1 - sum(all previous values))]

2. `float` column wt_split_column: re-calculated event weights
    - scaled from the original weights while making sure that across all events in a split, the total sum is the same as before the split
    - for example, for `fractions=[0.5]`, the dataset will be split in two equally large subsets, with values in `wt_split_column` being twice as large as in `source_weight_column`

Optional arguments:

- (str) `split_column`: name of the new column for storing the split value 
- (str) `wt_split_column`: name of the new column for scaled event weights
- (str) `source_weight_column`: name of the column of weights to scale

### WriteMVAData

TODO

### WriteROOTSnapshotAction

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