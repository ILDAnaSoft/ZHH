# ZHH Config Files

## preselection_cuts_cd.json
a single point of truth, they are
Values read by the PreSelection processor. 

## custom_statistics.json

This file can be used to fine-tune the statistics which may be used for job submissions. This is usefull

A list of lists of following structure:

```json
[x:number, [
    "process1", "process2", ...
], y:str]
```

where `y` defines the reference (either `total` or `expected`) and `x` gives a multiplier.

