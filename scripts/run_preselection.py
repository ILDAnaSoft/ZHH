import argparse
from zhh import EventCategories, CutflowProcessor, \
    categorize_6q, categorize_2l4q, categorize_4fsl, categorize_llhh, categorize_vvhh
import numpy as np
from tqdm.auto import tqdm
from zhh import DataSource, EventCategories, zhh_cuts

# The following dictionary defines how event categories should be managed
# It consists of a tuple of [
#   1. callback function given an DataSource to assign final states. see FinalStateDefinitions.py and the FinalStateDefinition type,
#   2. the default class to be assigned,
#   3. an optional category to be used. ignored if None. if [], will only assign the default class to all events  
#]

final_state_config = {
    'f4sl': (categorize_4fsl, EventCategories.F4_OTHER, None),
    'l2q4': (categorize_2l4q, EventCategories.F6_OTHER, None),
    'llhh': (categorize_llhh, None, None),
    'q6': (categorize_6q, EventCategories.qqqqqq, []),
    'vvhh': (categorize_vvhh, None, None)
}

hypotheses = {
    'mumuHHbbbb': ()
}

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    
    parser.add_argument('--sample', type=str, action='extend',
                        default=None,
                        help='--sample <path to AnalysisCombine result>:<name of final state config>')
    
    parser.add_argument('f4sl', type=str,
                        default=None,
                        help='Path to AnalysisCombine output directory for 4fsl; eg. $DATA_ROOT/AnalysisCombine/550-4fsl-fast-perf')
    
    parser.add_argument('l2q4', type=str,
                        default='/data/dust/user/bliewert/zhh/AnalysisCombine.old/550-2l4q-fast-perf',
                        help='Path to AnalysisCombine output directory of 2l4q')

    parser.add_argument('llhh', type=str,
                        default='/data/dust/user/bliewert/zhh/AnalysisCombine.old/550-llhh-fast-perf',
                        help='Path to AnalysisCombine output directory of llhh')
    
    parser.add_argument('q6', type=str,
                        default='/data/dust/user/bliewert/zhh/AnalysisCombine.old/550-6q-fast-perf',
                        help='Path to AnalysisCombine output directory of 6q')
    
    parser.add_argument('output', type=str,
                        default=None,
                        help='Path to a directory where output plots, tables and ROOT files will be stored. Will be this directory if not given.')
    
    parser.add_argument('hypothesis', type=str, choices=['μμbbbb', 'eebbbb', 'vv', 'qq'],
                        default='μμbbbb',
                        help='Which hypothesis to use.')
    
    args = parser.parse_args()

    HYPOTHESIS = args.hypothesis

    f4sl = DataSource(args.f4sl, '4fsl')
    l2q4 = DataSource(args.l2q4, '2l4q')
    llhh = DataSource(args.llhh, 'llhh')
    q6 = DataSource(args.q6, '6q')

    sources = [f4sl, l2q4, llhh, q6]

    for source in tqdm(sources):
        source.combine()
        source.fetchData()
        source.weight()

    # Check that no errors have occured
    for source in sources:
        assert((np.array(source.getTTree()['error_code'].array()) == 0).all())

    # categorize each event based on output of FinalStateRecorder
    for source in sources:
        analysis_channel, cat_fn, default_category, category_order = final_state_config[source.getName()]
        
        # registers the event categories
        cat_fn(analysis_channel)
        
        # parses the final state counts, find the first matching category,
        # or set to default_category (if not None)
        # if category_order is [], the default category (if given) is
        # applied to all events (if given)
        analysis_channel.evaluateEventCategories(default_category=default_category, order=category_order)

    pp = CutflowProcessor(sources, hypothesis=HYPOTHESIS, cuts=zhh_cuts(HYPOTHESIS), signal_categories=[
        EventCategories.μμHHbbbb
    ])

    masks, subsets, last_calc_dicts = pp.process()

    print('Creating cutflow plots...')
    pp.cutflowPlots(display=False)
    
    print('Creating cutflow tables...')
    pp.cutflowTable([
        ('eebb', 'eebb'),
        ('\\boldsymbol{\\mu\\mu} bb', 'μμbb'),
        #('lvbbqq', mask_lvbbqq, l2q4, ''),
        ('\\boldsymbol{e \\nu b b q q} ', 'evbbqq'),
        ('\\boldsymbol{\\mu \\nu b b q q} ', 'μvbbqq'),
        ('\\boldsymbol{\\tau \\nu b b q q} ', 'τvbbqq'),
        ('bbqqqq', 'bbqqqq'),
        ('llbbbb', 'llbbbb'),
        ('llqqh', 'llqqh'),
        ('llHH', 'llhh'),
        ('(llbbbb)', 'llhh_llbbbb'),
        ('(eebbbb)', 'eeHHbbbb'),
        ('(\\mu\\mu bbbb)', 'μμHHbbbb'),
    ])
    
    print('Writing ROOT file with weights, event mask and event category...')
    pp.writeROOTFiles()