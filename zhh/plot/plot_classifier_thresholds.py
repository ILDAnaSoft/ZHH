import matplotlib.pyplot as plt
import numpy as np

def plot_classifier_thresholds(stats:np.ndarray, title:str='MVA Score'):
    from zhh import MulticlassModel, fig_ild_style
    from zhh.plot.ild_style import legend_kwargs_fn
    
    figs = []
    
    threshold_scan = stats['threshold']

    for zoomed_in, xlim in [
        (False, (0., 1.)),
        (True, (0.9, 1.))
    ]:
        fig, ax1 = plt.subplots()
        ax1.set_ylabel('Efficiency, Purity')
        ax1.set_xlim(xlim)
        
        ax2 = ax1.twinx()
        ax2.set_ylabel('Significance')
        
        best_sig = np.nanmax(stats['significance'])
        best_sig_at = threshold_scan[np.nanargmax(stats['significance'])]

        lns1 = ax1.plot(threshold_scan, stats['efficiency'], label='Efficiency')
        lns2 = ax1.plot(threshold_scan, stats['purity'], label='Purity')
        
        lns3 = ax2.plot(threshold_scan, stats['significance'], label=f'Significance' + (f' [max.: {best_sig:.3f} @ {best_sig_at:.6f}]' if xlim[0] <= best_sig_at <= xlim[1] else ''), color='green')
        if xlim[0] <= best_sig_at <= xlim[1]:
            ax1.axvline(x=best_sig_at, color='red')
        
        lns = lns1+lns2+lns3
        ax1.legend(lns, [str(l.get_label()) for l in lns], loc='lower left', **legend_kwargs_fn())
        ax1.grid()
        
        fig_ild_style(fig, title=title, xlabel='Cut Value', xunit=None,
                        yunit=None, ylabel_prefix='', ild_offset_x=0.65, ild_offset_y=0.12);
        
        if zoomed_in:
            ax1.set_xscale('function', functions=(MulticlassModel.transform_forward, MulticlassModel.transform_inverse))
            
            xticks = [0.9, 0.99, 0.999, 0.9999, 0.99999, 0.999999]
            ax1.set_xticks(xticks, labels=[str(xt) for xt in xticks], minor=False, fontsize=9)
            ax1.minorticks_off()
            
        figs.append(fig)
    
    return figs