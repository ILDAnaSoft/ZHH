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

### How to run the analysis
```shell
Marlin scripts/ZHHllbbbbAnalysis.xml
```
