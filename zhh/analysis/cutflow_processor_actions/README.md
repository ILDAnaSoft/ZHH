# Documentation of CutflowProcessorActions

The cutflow tool is used for statistical analysis on large-scale, ROOT-based analysis. However, on it's own, it only manages data IO (i.e. input of feature data in ROOT TTrees), event categorization and weighting. Anything beyond that must be implemented as CutflowProcessorAction. These are classes implementing desired functionalities such as plotting, MVA analysis etc.

For a documentation about the configuration of `cutflow`, see `config/README.md`.

## Base classes

Any action must implement at least `CutflowProcessorAction`. If you want to implement a custom action, make sure to import it. Only available subclasses of `CutflowProcessorAction` (or any action inheriting from it) can be used in cutflow.

### CutflowProcessorAction

Each action must implement a `run()` method that does the actual job, a `complete()` method that returns whether or not the action has completed it's job (e.g. by checking whether an output file exists, a dataset exists in each DataStore etc.), and, optionally, a `reset()` method that cleans up the work by `run()`.

Default parameters:
- (bool) `transforms_data`: whether or not the action creates any files or writes data using the CutflowProcessor. If False, the action will always execute. Useful for printing debug information. If True, will only execute if a previous action is going to be executed or the `complete()` of this action did not return True. Defaults to True.
- (bool) `interruptible`: whether or not an action can be interrupted (e.g. by KeyboardInterrupt, CTRL+C). If False, `action.reset()` will be called upon interrupt. Useful to abort file IO with HDF5 files to avoid data corruption (requires the action to implement `reset()` in a resonable way). Defaults to True.

### FileBasedProcessorAction

If an action is supposed to create a file output, the `FileBasedProcessorAction` can be used. It requires implementing an `output()` method that should return either one `LocalTarget` or a list of `LocalTargets`, similar to a law task.

## Abstract classes

By inheriting from these abstract classes and implementing them, it is possible to provide added functionality.

### MVAThresholdFinderInterface

Allows an MVA optimizer to dynamically supply a threshold value so that a cut on the optimal value can be defined without the cut value being known. Use the syntax `<mva_name>.threshold` in a cut to accomplish this. For example, with a MVA `default_mva`, the optimal value can be referenced in a greater-equal-cut as `default_mva.threshold`.

Must implement: `findThreshold()`

Deriving classes:
- [SklearnMulticlassTraining](#sklearnmulticlasstraining)
- [SklearnMulticlassHyperparamTraining](#sklearnmulticlasshyperparamtraining)

### CutGroupProviderInterface

Allows an action (e.g. an MVA optimizer) to dynamically add a cut group to the CutflowProcessor.

Must implement: `fetchCutGroup()->list[ValueCut]`

Deriving classes:
- [OptimizeMVANDimensional](#optimizemvandimensional)

## Implementations

The following list gives an overview of the `CutflowProcessorActions` packaged together with cutflow.

### ApplyCuts

TODO

### CreateCutflowPlots

TODO

### CreateCutflowTable

TODO

### OptimizeMVANDimensional

TODO

### PlotMVAFeatures

Creates plots of all the input distributions for MVA training. Uses the data written out by [WriteMVAData](#writemvadata).

Required arguments:

- (str) `mva`: name of the MVA registered in cutflow
- (str) `file`: name of the output PDF file

Optional arguments:
- (Literal['train', 'test']) `plot_which`: which dataset should be plotted. Defaults to 'train'.
- (dict) `plot_kwargs`: base options to control the plotting by plot_weighted_hist(). other plotting optins are merged into a copy of this. Defaults to an empty dict.

### PlotMVAFeatures

TODO

### PlotObservable

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

Runs inference for a MVA registered with `cutflow` for a selection of events. Requires the MVA to have been trained before (see, e.g., [SklearnMulticlassTraining](#sklearnmulticlasstraining)). Writes out one total signal classifier variable at column `mva_spec['output_label']` (from a sum of individual classes) as well as all individual class outputs.

Individual class outputs are written to `{mva_spec['output_label']}#{category}` for each category registered in `mva_spec['classes']`.

Data for events which do not match the selection will be set to zero.

Required arguments:

- (str) `mva`: name of the MVA registered in cutflow
- (int) `split`: defines the split of events to run inference for. e.g. 0 for training dataset, 1 for testing. See SplitDatasets for more info

Optional arguments:

- (int|None) `step`: defines the cut group to select the events to run inference for. if None, will use the last registered cut group. Defaults to None.
- (str) `clf_prop`: key at which the classifier is saved in the output pickle file. Defaults to 'clf'
- (bool) `overwrite`: whether or not to overwrite already potentially existing output columns. Defaults to True.

### SplitDatasets

Splits the dataset in n smaller subsets, e.g. for train/testing. The splitting is done accross all steps, i.e. regardless of cut group.

The action creates two new columns defined by `split_column` and `wt_split_column`.

1. `uint8` column split_column; for n entries in fractions:
    - value 0 is assigned to a fraction of events specified at position 0 in fractions
    - value `1[..]n-1` to fractions given at position `1[..]n-1`
    - value n is assigned to the remaining events [fraction (1 - sum(all previous values))]

2. `float` column wt_split_column: re-calculated event weights
    - scaled from the original weights while making sure that across all events in a split, the total sum is the same as before the split
    - for example, for `fractions=[0.5]`, the dataset will be split in two equally large subsets, with values in `wt_split_column` being twice as large as in `source_weight_column`

Optional arguments:

- (str) `split_column`: name of the new column for storing the split value. Defaults to `split`.
- (str) `wt_split_column`: name of the new column for scaled event weights. Defaults to `weights_split`.
- (str) `source_weight_column`: name of the column of weights to scale. Defaults to `weight`.

### WriteMVAData

TODO

### WriteROOTSnapshot

Writes out column-wise data from all data sources to ROOT TTrees in `file.` The columns to write must be explicitly stated in `include_custom_columns` or, input/MVA outputs columns may be added automatically by setting `include_input_columns` / `include_mva_outputs`. An additional column `source` is always written. It stores an uint8 specifying the index of the `DataSource` (as registered in steer['sources']) an event belongs to.   

Required arguments:

- (str) `file`: name of the output ROOT file. file will be re-created if the action is executed again.

Optional arguments:
- (str) `tree`: name of the output TTree. Defaults to `cutflow`.
- (list[str]) `include_custom_columns`: list of columns to write out. Defaults to [].
- (bool) `include_input_columns`: if True, all registered inputs from the steering file's `interpret` section will be stored. Defaults to `False`.
- (bool) `include_mva_outputs`: if True, all MVA output distributions will be stored. Defaults to `False`.
- (int) `CHUNK_SIZE`: Data is written in batches of `CHUNK_SIZE` rows for each of the `n` selected columns at a time (i.e. `n x CHUNK_SIZE` per batch). Defaults to 65536.