# Job submission at KEK

Use a terminal to `cd` into this directory. Then, use `./submit <JOB_NAME>` to first generate the submission scripts from templates, and then to confirm their submission. The environment variable `$DATA_PATH` is used to defined where all the results are to be stored. It will be set to `/group/ilc/users/$(whoami)/jobresults` if not set.

Possible tasks are:

- 250-ftag-sgv; input files: all 250 GeV samples from the mc-2020 production
- 550-hh-sgv; input files: 

Both run SGV on all 

# Problems
It seems that on KEK CC the necessary libraries `blas` and `lapack` are not automatically loaded when running SGV using this setup. You can check that be running `./usesgvlcio` and see if the output `sgvout.slcio` contains only an empty `MarlinTrkTracks` collection. If so, add the following to `sgvenv.sh`:

    export LD_PRELOAD="/usr/lib64/libblas.so:/usr/lib64/liblapack.so:$LD_PRELOAD"

# Used environment variables

The following are used internally and default to values which match the default zhh install setup.

 Variable          | Default value                  
-------------------|--------------------------------
 SGV_PRODUCED_FILE | sgvout.slcio                   
 SGV_EXECUTABLE    | $SGV_DIR/tests/usesgvlcio.exe  
 SGV_INPUT_FILE    | input.slcio                    

