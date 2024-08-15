# ZHH
Code for ZHH analysis with ILD

### Setup library paths
From the top level of the repository execute
```shell
source setup.sh
```
You can verify the path by doing
```shell
echo $MARLIN_DLL
```

### Compilation and installation of processors
Ensure that you have setup the above library paths.

For each processor under `source/`, do
```shell
mkdir build
cd build
cmake -C $ILCSOFT/ILCSoft.cmake ..
make install
```

#### Helper script
If you compile a freshly cloned copy from scratch, you _might_ want to use the `compile_from_scratch.sh` script.
For that you need to `source` it from the top level directory of the repository. It saves some typing, that's all.

### Running the full analysis
We use the luigi analysis framework [LAW](https://github.com/riga/law) to orchestrate the execution of batch jobs and book-keeping of results etc. As of now, we use [this](https://github.com/riga/law/commit/673c2ac16eb8da9304a6c749e557f9c42ad4d976) commit. A specific version of LAW can be installed using `git install git+https://github.com/riga/law`.

To prepare law for submission of jobs, ```cd``` into ```workflows``` and ```source setup.sh```.

To run the preselection analysis, use

```shell
law run PreselectionFinal --transfer-logs --poll-interval=120sec
```

### Task Overview

| Task name                 | Description           | Parameters with defaults |
|---------------------------|-----------------------|--------------------------|
| CreateRawIndex            | Creates an index of all readable sample files and physics processes associated to them. See ProcessIndex. | - |
| CreatePreselectionChunks  | Slices the sample files into chunks according to a desired normalization, physics sample size and duration per job. | - |
| PreselectionRuntime       | Runs the Marlin analysis for each proc_pol combination over 50 events to estimate the runtime per event. | - |
| PreselectionFinal         | Runs the Marlin analysis with the chunking as given above.  | - |
| PreselectionSummary       | Creates summary plots for events passing the preselection cuts, their kinematic distributions etc.  ||

### Running the analysis on individual files

The analysis runs [Marlin](https://github.com/iLCSoft/Marlin) with a steering file covering the llHH, vvHH and qqHH channels at once, with individual options for jet clustering, ISR recovery + lepton pairing and hypothesis-dependent cuts.

```shell
Marlin scripts/ZHH_v2.xml
```
