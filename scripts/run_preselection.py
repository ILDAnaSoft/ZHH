import argparse
from zhh import EventCategories, PreselectionProcessor, \
    categorize_6q, categorize_2l4q, categorize_llbb, categorize_llhh
import numpy as np
from tqdm.auto import tqdm
from zhh import AnalysisChannel, EventCategories

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument('--llbb', type=str,
                        default='/data/dust/user/bliewert/zhh/AnalysisCombine/550-llbb-fast-perf',
                        help='Path to AnalaysisCombine output directory for llbb')
    
    parser.add_argument('--l2q4', type=str,
                        default='/data/dust/user/bliewert/zhh/AnalysisCombine/550-2l4q-fast-perf.old',
                        help='Path to AnalaysisCombine output directory of 2l4q')

    parser.add_argument('--llhh', type=str,
                        default='/data/dust/user/bliewert/zhh/AnalysisCombine/550-llhh-fast-perf',
                        help='Path to AnalaysisCombine output directory of llhh')
    
    parser.add_argument('--q6', type=str,
                        default='/data/dust/user/bliewert/zhh/AnalysisCombine/550-6q-fast-perf',
                        help='Path to AnalaysisCombine output directory of 6q')
    
    parser.add_argument('--output', type=str,
                        default=None,
                        help='Path to a directory where output plots, tables and ROOT files will be stored. Will be this directory if not given.')
    
    args = parser.parse_args()

    PRESELECTION = 'll'

    llbb = AnalysisChannel(args.llbb, 'llbb')
    l2q4 = AnalysisChannel(args.l2q4, '2l4q')
    llhh = AnalysisChannel(args.llhh, 'llhh')
    q6 = AnalysisChannel(args.q6, '6q')

    sources = [llbb, l2q4, llhh, q6]

    for source in tqdm(sources):
        source.combine()
        source.fetchPreselection(PRESELECTION)
        source.weight()

    # Check that no errors have occured
    for source in sources:
        assert((np.array(source.getTTree()['error_code'].array()) == 0).all())

    # categorize each event based on output of FinalStateRecorder
    for analysis_channel, cat_fn, default_category, category_order in [
        (llbb, categorize_llbb, EventCategories.llbb, None),
        (l2q4, categorize_2l4q, EventCategories.F6_OTHER, None),
        (llhh, categorize_llhh, None, None),
        (q6, categorize_6q, EventCategories.qqqqqq, [])
    ]:
        # registers the event categories
        cat_fn(analysis_channel)
        
        # parses the final state counts, find the first matching category,
        # or set to default_category (if not None)
        # if category_order is [], the default category (if given) is
        # applied to all events (if given)
        analysis_channel.evaluateEventCategories(default_category=default_category, order=category_order)

    pp = PreselectionProcessor(sources)

    masks, subsets, last_calc_dicts = pp.process(signal_categories=[
        EventCategories.eeHHbbbb, 
        EventCategories.µµHHbbbb,
        EventCategories.ττHHbbbb,
        #EventCategories.llHH
    ])

    print('Creating cutflow plots...')
    pp.cutflowPlots(display=False)
    
    print('Creating cutflow tables...')
    pp.cutflowTable([
        ('eebb', 'eebb'),
        ('\\boldsymbol{\\mu\\mu} bb', 'µµbb'),
        #('lvbbqq', mask_lvbbqq, l2q4, ''),
        ('\\boldsymbol{e \\nu b b q q} ', 'evbbqq'),
        ('\\boldsymbol{\\mu \\nu b b q q} ', 'µvbbqq'),
        ('\\boldsymbol{\\tau \\nu b b q q} ', 'τvbbqq'),
        ('bbqqqq', 'bbqqqq'),
        ('llbbbb', 'llbbbb'),
        ('llqqH', 'llqqh'),
        ('llHH', 'llhh'),
        ('(llbbbb)', 'llhh_llbbbb'),
        ('(eebbbb)', 'eeHHbbbb'),
        ('(\\mu\\mu bbbb)', 'µµHHbbbb'),
    ])
    
    print('Writing ROOT file with weights, event mask and event categy...')
    pp.writeROOTFiles()