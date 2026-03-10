from zhh import cutflow_parse_steering_file, cutflow_process_steering, cutflow_initialize_sources, cutflow_parse_cuts, CutflowProcessor, \
    cutflow_parse_actions, cutflow_execute_actions, cutflow_register_mvas
import argparse, yaml, logging
from os import environ

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('steer', type=str, default=None, help='Path to a YAML steering file, e.g. $REPO_ROOT/config/llHHbbbb.yaml')
    parser.add_argument('--skip_integrity_check', action='store_true', help='Controls whether the check of matching ROOT and HDF5 files should be skipped. Defaults to False.')
    parser.add_argument('--log_level', type=str, default='DEBUG', help='Must be any of CRITICAL, FATAL/CRITICAL, ERROR, WARNING, WARN/WARNING, INFO, DEBUG or NOTSET. Defaults to DEBUG.')
    parser.add_argument('--reset', action='store_true', help='Controls whether or not to reset the final state definitions and weightings of each source')
    parser.add_argument('--readonly', action='store_true', help='If set, will only attempt to only use read operation when interacting with HDF5 files. Will fail if features from ROOT TTrees should be used which have not yet been converted. For debug use only.')

    args = parser.parse_args()
    log_level = getattr(logging, args.log_level)
    
    # process the steering file -> sources and final state info
    print("-----------------------LADIDA process steering file -----------------------------")
    steer = cutflow_parse_steering_file(args.steer)
    sources_map, final_state_configs, reset_sources = cutflow_process_steering(steer, integrity_check=not args.skip_integrity_check,
                                                                               check_requires_exact_path_match=not args.skip_integrity_check,
                                                                               readonly=args.readonly)
    
    if args.reset:
        reset_sources = list(sources_map.keys())

    # initialize all sources
    print("-----------------------LADIDA initialize -----------------------------")
    sources = list(sources_map.values())
    cutflow_initialize_sources(sources, final_state_configs, lumi_inv_ab=steer['luminosity'], reset_sources=reset_sources)
    
    # parse the cuts and combine all info to a CutflowProcessor
    print("-----------------------LADIDA parse cuts and combine info -----------------------------")
    preselection = cutflow_parse_cuts(steer['cuts']['preselection'])
    cp = CutflowProcessor(sources, hypothesis=steer['hypothesis'], cuts=preselection, signal_categories=steer['signal_categories'])
    cutflow_register_mvas(steer, cp)

    # prepare the cutflow processor
    print("-----------------------LADIDA prepare cutflow processor -----------------------------")
    actions = cutflow_parse_actions(steer, cp)

    # dry run required for full run
    print("-----------------------LADIDA test run -----------------------------")
    print('Going to execute following actions:')
    to_execute = cutflow_execute_actions(actions, check_only=True)
    for i, action in enumerate(to_execute):
        print(f'Action {i+1}/{len(to_execute)}:', action)
    print("-----------------------LADIDA full run -----------------------------")
    cutflow_execute_actions(actions, log_level=log_level)
