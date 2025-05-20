# example usage: python analyze_ftag.py --fast-sim-dir=/data/dust/user/$(whoami)/zhh/AnalysisRuntime/250-ftag-fast-pfl --full-sim-dir=/data/dust/user/$(whoami)/zhh/AnalysisRuntime/250-ftag-full

import uproot as ur
import argparse
from glob import glob
import numpy as np
import matplotlib.pyplot as plt
from tqdm.auto import tqdm
from phc import export_figures
from sklearn.metrics import roc_curve, auc
from copy import deepcopy
from zhh import PlotContext, colormap_desy, plot_weighted_hist, fig_ild_style
from zhh.plot.ild_style import ild_style_defaults

if __name__=="__main__":
    parser = argparse.ArgumentParser(description='Creates plots to analyze flavor tagging performance between LCFIPlus/ParT and fast/full simulation. Requires a subsequent run of Marlin with dev_flavortag_compare.xml and the JetTaggingComparison processor. Produces each two PDFs for fast vs full sim and LCFIPlus vs ParT performance.')
    parser.add_argument('--fast-sim-dir', type=str, required=False, help='directory with ROOT files from Marlin with fast sim files')
    parser.add_argument('--full-sim-dir', type=str, required=False, help='directory with ROOT files from Marlin with full sim files')
    
    args = parser.parse_args()
    
    fast_sim_dirn = str(args.fast_sim_dir)
    full_sim_dirn = str(args.full_sim_dir)

    #fast_sim_dirn = '/data/dust/user/bliewert/zhh/AnalysisRuntime/250-ftag-fast-pfl'
    #full_sim_dirn = '/data/dust/user/bliewert/zhh/AnalysisRuntime/250-ftag-full'

    tot_length_fast_sim = 0
    tot_length_full_sim = 0

    files_fast_sim = glob(f'{fast_sim_dirn}/*.root')
    files_full_sim = glob(f'{full_sim_dirn}/*.root')

    n_features = 8

    for f in tqdm(files_fast_sim):
        with ur.open(f) as rf:
            length = rf['JetTaggingComparison'].num_entries // 4
            tot_length_fast_sim += length
            
    for f in tqdm(files_full_sim):
        with ur.open(f) as rf:
            length = rf['JetTaggingComparison'].num_entries // 4
            tot_length_full_sim += length
            
    print(f'Total lengths for fast and full simulation: {tot_length_fast_sim}, {tot_length_full_sim}')

    tags_full_sim = np.zeros((tot_length_full_sim, n_features))
    tags_fast_sim = np.zeros((tot_length_fast_sim, n_features))

    labels_full_sim = np.zeros(tot_length_full_sim, dtype='B')
    labels_fast_sim = np.zeros(tot_length_fast_sim, dtype='B')

    counter_fast_sim = 0
    counter_full_sim = 0

    def get_label(path:str):
        if 'zz_bbbb' in path:
            return 0
        elif 'zz_cccc' in path:
            return 1
        elif 'zz_ssss' in path:
            return 2
        elif 'zz_uuuu' in path or 'zz_dddd' in path:
            return 3
        else: raise Exception(f'No label for path: {path}')
            
    for tags, labels, files in [
        (tags_full_sim, labels_full_sim, files_full_sim),
        (tags_fast_sim, labels_fast_sim, files_fast_sim)
    ]:
        counter = 0
        
        for f in tqdm(files):
            with ur.open(f) as rf:
                length = rf['JetTaggingComparison'].num_entries // 4
                
                part_b = np.array(rf['JetTaggingComparison']['tags1'].array())[:, 0]
                lcfi_b = np.array(rf['JetTaggingComparison']['tags2'].array())[:, 0]
                
                if len(part_b) % 4 == 0 and len(lcfi_b) % 4 == 0:
                    part_b = part_b.reshape(-1, 4)
                    lcfi_b = lcfi_b.reshape(-1, 4)

                    part_b.sort(axis=1)
                    lcfi_b.sort(axis=1)
                    
                    tags[counter:counter + length, :4] = part_b
                    tags[counter:counter + length, 4:] = lcfi_b
                    
                    labels[counter:counter + length] = get_label(f)
                    
                    counter += length
                else:
                    print(f'Skipping file {f}')
        
        if tags is tags_fast_sim:
            counter_fast_sim = counter
        else:
            counter_full_sim = counter

    tags_full_sim = tags_full_sim[:counter_full_sim]
    tags_fast_sim = tags_fast_sim[:counter_fast_sim]

    labels_full_sim_raw = labels_full_sim[:counter_full_sim]
    labels_fast_sim_raw = labels_fast_sim[:counter_fast_sim]

    print(f'{tags_full_sim.shape=}, {tags_fast_sim.shape=}')

    labels_fast_sim = np.copy(labels_fast_sim_raw)
    labels_full_sim = np.copy(labels_full_sim_raw)

    labels_fast_sim[labels_fast_sim > 0] = 1
    labels_full_sim[labels_full_sim > 0] = 1

    # bbbb vs qqqq
    context = PlotContext(colormap_desy)

    label_sig = 'bbbb'
    label_bkg = 'qqqq'

    hist_kwargs_overwrite = {
        label_sig: {  },
        label_bkg: { 'histtype': 'stepfilled', 'color': context.getColorByKey(label_bkg) }
    }
    plot_kwargs = {
        'xlim': [-.03, 1.03],
        'yscale': 'log',
        'plot_hist_kwargs': {
            'stacked': False,
            'show_stats': False,
            'hist_kwargs': { 'hatch': None },
            'ylim': [1, 1e6]
        },
        'ild_style_kwargs': {
            'legend_kwargs': { 'loc': 'upper right', 'bbox_to_anchor': (.98, .98), 'fancybox': False }
        }
    }

    figs = []

    for sim_label, inputs, labels in [
        ('Fast Sim', tags_fast_sim, labels_fast_sim),
        ('Full Sim', tags_full_sim, labels_full_sim)
    ]:     
        for source_label, source_shift in [
            ('ParT', 0),
            ('LCFIPlus', 4)
        ]:
            for bmax_idx in range(4):
                feature_label = f'bmax{bmax_idx + 1}'
                
                plot_dict = {}
                
                plot_dict[label_sig] = ( inputs[labels == 0, source_shift + bmax_idx], None )
                plot_dict[label_bkg] = ( inputs[labels == 1, source_shift + bmax_idx], None )
                
                plot_kwargs_current = deepcopy(plot_kwargs)
                
                fig = plot_weighted_hist(plot_dict, title=f'{sim_label} {source_label} <{feature_label}>', xlabel=feature_label, xunit=None,
                                            plot_context=context, plot_hist_kwargs_overwrite=hist_kwargs_overwrite, **plot_kwargs_current)
                
                figs.append(fig)
                
    export_figures('flavor_tag_distributions_bbbb_vs_qqqq.pdf', figs)

    # fast vs full sim
    context = PlotContext(colormap_desy)

    label_sig = 'Fast Sim'
    label_bkg = 'Full Sim'

    hist_kwargs_overwrite = {
        label_sig    : {  },
        label_bkg: { 'histtype': 'stepfilled', 'color': context.getColorByKey(label_bkg) }
    }
    plot_kwargs = {
        'xlim': [-.03, 1.03],
        'yscale': 'log',
        'plot_hist_kwargs': {
            'stacked': False,
            'show_stats': False,
            'hist_kwargs': { 'hatch': None },
            'ylim': [1, 1e6],
            'custom_styling': False
        },
        'ild_style_kwargs': {
            'legend_kwargs': { 'loc': 'upper right', 'bbox_to_anchor': (.98, .98), 'fancybox': False }
        }
    }

    figs = []

    for bmax_idx in range(4):
        feature_label = f'bmax{bmax_idx + 1}'
        
        for source_label, source_shift in [
            ('ParT', 0),
            ('LCFIPlus', 4)
        ]:
            for event_type, event_label in [
                ('bbbb', 0),
                ('qqqq', 1)
            ]:
                plot_dict = {}
                
                max_event_count = max(np.sum(labels_fast_sim == event_label), np.sum(labels_full_sim == event_label))
                
                for sim_label, inputs, labels in [
                    (label_sig, tags_fast_sim, labels_fast_sim),
                    (label_bkg, tags_full_sim, labels_full_sim)
                ]:
                    length = np.sum(labels == event_label)
                    weight = None if length == max_event_count else np.ones(length) *  max_event_count/length
                    plot_dict[sim_label] = ( inputs[labels == event_label, source_shift + bmax_idx], weight )
                    
                plot_kwargs_current = deepcopy(plot_kwargs)
                
                fig = plot_weighted_hist(plot_dict, title=f'{source_label} {event_type} <{feature_label}>', xlabel=feature_label, xunit=None,
                                            plot_context=context, plot_hist_kwargs_overwrite=hist_kwargs_overwrite, **plot_kwargs_current)
                
                figs.append(fig)
                
    export_figures('flavor_tag_distributions_fast_vs_full_sim.pdf', figs)

    # fast vs full sim
    nth_name = ['highest', '2nd highest', '3rd highest', '4th highest']
    
    label_sig = 'bbbb'
    label_bkg = 'qqqq'
    
    figs = []

    for bmax_index in range(1, 5):
        nth = nth_name[bmax_index - 1] 
        bmax_label = f'bmax{bmax_index}'

        for source_index, source_label in [(0, 'ParT'), (1, 'LCFIPlus')]:  
            fig, ax = plt.subplots(figsize=(6, 5))
            
            for sim_label, inputs, labels in [
                ('Fast Sim', tags_fast_sim, labels_fast_sim),
                ('Full Sim', tags_full_sim, labels_full_sim)
            ]:
                fpr, tpr, thresholds = roc_curve(labels, inputs[:, source_index*4 + bmax_index-1], pos_label=0)
                ax.plot(fpr, tpr, label=f'{sim_label} (ROC-AUC = {auc(fpr, tpr):.4f})')

            ax.set_ylim(1e-6, 1)
            
            fig_ild_style(fig, xlim=[1e-6, 1],
                    title=f'{source_label}: Separating {label_sig} vs {label_bkg} with {nth} btag',
                    xlabel='False Positive Rate', xunit=None, xscale='log',
                    yunit='',
                    ylabel_prefix='True Positive Rate', beam_spec=False)
            
            #ax.plot([0, 1], [0, 1], 'k--', label='Random guess')
            ax.legend(loc='lower right', prop= { 'family': ild_style_defaults['fontname'], 'size': 9 }, bbox_to_anchor=(.96, .04), fancybox=False)
            ax.grid()
            
            figs.append(fig)
            
    export_figures('flavor_tag_roc_fast_vs_full_sim.pdf', figs)

    # LCFIPlus vs ParT
    figs = []

    for bmax_index in range(1, 5):
        nth = nth_name[bmax_index - 1] 
        bmax_label = f'bmax{bmax_index}'
        
        for sim_label, inputs, labels in [
            ('Fast Sim', tags_fast_sim, labels_fast_sim),
            ('Full Sim', tags_full_sim, labels_full_sim)
        ]:
        
            fig, ax = plt.subplots(figsize=(6, 5))
            
            for source_index, source_label in [(0, 'ParT'), (1, 'LCFIPlus')]:  
                fpr, tpr, thresholds = roc_curve(labels, inputs[:, source_index*4 + bmax_index-1], pos_label=0)
                ax.plot(fpr, tpr, label=f'{source_label} (ROC-AUC = {auc(fpr, tpr):.4f})')

            ax.set_ylim(1e-6, 1)
            
            fig_ild_style(fig, xlim=[1e-6, 1],
                    title=f'{sim_label}: Separating {label_sig} vs {label_bkg} with {nth} btag',
                    xlabel='False Positive Rate', xunit=None, xscale='log',
                    yunit='',
                    ylabel_prefix='True Positive Rate', beam_spec=False)
            
            #ax.plot([0, 1], [0, 1], 'k--', label='Random guess')
            ax.legend(loc='lower right', prop= { 'family': ild_style_defaults['fontname'], 'size': 9 }, bbox_to_anchor=(.96, .04), fancybox=False)
            ax.grid()
            
            figs.append(fig)
            
    export_figures('flavor_tag_roc_part_vs_lcfiplus.pdf', figs)
    
    #
    # LCFIPlus vs ParT AND fast vs full sim
    figs = []

    for bmax_index in range(1, 5):
        nth = nth_name[bmax_index - 1] 
            
        fig, ax = plt.subplots(figsize=(6, 5))
        
        for source_index, source_label in [(0, 'ParT'), (1, 'LCFIPlus')]:  
            for sim_label, inputs, labels in [
                ('Fast Sim', tags_fast_sim, labels_fast_sim),
                ('Full Sim', tags_full_sim, labels_full_sim)
            ]:
                is_full_sim = sim_label == 'Full Sim'
                
                fpr, tpr, thresholds = roc_curve(labels, inputs[:, source_index*4 + bmax_index-1], pos_label=0)
                ax.plot(fpr, tpr, label=f'{sim_label} {source_label} (AUROC = {auc(fpr, tpr):.3f})',
                        color=colormap_desy.colors[source_index],
                        linestyle=('dashed' if is_full_sim else 'solid'))

        ax.set_ylim(0, 1)
        
        fig_ild_style(fig, xlim=[1e-6, 1],
                    title=f'Separating {label_sig} vs {label_bkg} with {nth} btag',
                    xlabel='False Positive Rate', xunit=None, xscale='log',
                    yunit='',
                    ylabel_prefix='True Positive Rate', beam_spec=False)
        
        ax.legend(loc='lower right', prop= { 'family': ild_style_defaults['fontname'], 'size': 9 }, bbox_to_anchor=(.96, .04), fancybox=False)
        ax.grid()
        
        figs.append(fig)
        
    export_figures('flavor_tag_overall.pdf', figs)