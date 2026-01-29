from zhh import cutflow_parse_steering_file, cutflow_process_steering, cutflow_initialize_sources, cutflow_parse_cuts, CutflowProcessor, \
    cutflow_parse_actions, cutflow_execute_actions, cutflow_register_mvas
import argparse, yaml, logging

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('steer', type=str, default=None, help='Path to a YAML steering file, e.g. $REPO_ROOT/config/llHHbbbb.yaml')

    args = parser.parse_args()

    # process the steering file -> sources and final state info
    steer = cutflow_parse_steering_file(args.steer)
    sources_map, final_state_configs, reset_sources = cutflow_process_steering(steer)

    # initialize all sources
    sources = list(sources_map.values())
    cutflow_initialize_sources(sources, final_state_configs, lumi_inv_ab=steer['luminosity'], reset_sources=reset_sources)
    
    # parse the cuts and combine all info to a CutflowProcessor
    preselection = cutflow_parse_cuts(steer['preselection']['cuts'])
    cp = CutflowProcessor(sources, hypothesis=steer['hypothesis'], cuts=preselection, signal_categories=steer['signal_categories'])
    cutflow_register_mvas(steer, cp)

    #cp.process()

    # prepare the cutflow table
    actions = cutflow_parse_actions(steer, cp)

    # dry run required for full run
    cutflow_execute_actions(actions, check_only=True)

    cutflow_execute_actions(actions, log_level=logging.DEBUG)