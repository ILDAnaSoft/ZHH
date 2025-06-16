import numpy as np
import uproot as ur
import matplotlib.pyplot as plt

def plot_final_state_counts(tree):
    from zhh.analysis.PreselectionAnalysis import fs_columns
    
    pdg = np.array(tree[f'final_state_counts/final_state_counts.first'].array(entry_stop=1)[0])
    count = np.array(tree[f'final_state_counts/final_state_counts.second'].array()).sum(axis=0)
    xpos = np.arange(len(fs_columns))
    
    fig, ax = plt.subplots()
    ax.set_xticks(xpos)
    ax.set_xticklabels(fs_columns, rotation=45)
    ax.set_xlim(-1, len(fs_columns))
    ax.bar(xpos, count)
    ax.set_title('Final state counts')
    ax.set_yscale('log')