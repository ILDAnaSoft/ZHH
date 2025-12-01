# ZHH Config Files

## preselection_cuts_cd.json
a single point of truth, they are
Values read by the PreSelection processor. 

# Removed

## custom_statistics.json

**custom_statistics can now instead be set per configuration in workflows/configurations.py**

This file can be used to fine-tune the statistics which may be used for job submissions. This is usefull

A list of lists of following structure:

```json
[x:number, [
    "process1", "process2", ...
], y:str]
```

where `y` defines the reference (either `total` or `expected`) and `x` gives a multiplier.

