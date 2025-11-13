from zhh import parse_steering_file, process_steering, initialize_sources, parse_cuts, CutflowProcessor, parse_steer_cutflow_table
import argparse, yaml

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('steer', type=str, default=None, help='Path to a YAML steering file, e.g. $REPO_ROOT/config/llHHbbbb.yaml')

    args = parser.parse_args()

    # process the steering file -> sources and final state info
    steer = parse_steering_file(args.steer)
    sources_map, final_state_configs, reset_sources = process_steering(steer)

    # initialize all sources
    sources = list(sources_map.values())
    initialize_sources(sources, final_state_configs, lumi_inv_ab=steer['luminosity'], reset_sources=reset_sources)
    
    # parse the cuts and combine all info to a CutflowProcessor
    preselection = parse_cuts(steer['preselection']['cuts'])
    cp = CutflowProcessor(sources, hypothesis=steer['hypothesis'], cuts=preselection, signal_categories=steer['signal_categories'])

    # prepare the cutflow table
    preselection_cf_table = steer['preselection'].get('cutflow_table')
    if preselection_cf_table is not None:
        cutflow_table_items, cutflow_table_kwargs = parse_steer_cutflow_table(steer, filename=preselection_cf_table)

    