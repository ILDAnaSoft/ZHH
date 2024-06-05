import seaborn as sns
import matplotlib.pyplot as plt
from matplotlib.figure import Figure
from typing import Optional, List
import numpy as np

def plot_preselection_pass(vecs:np.ndarray) -> List[Optional[Figure]]:
    bars = list(range(len(vecs.T) + 1))
    vals = [len(vecs)]

    temp = np.ones(len(vecs), dtype=int)

    for i in range(len(vecs.T)):
        temp = temp & vecs.T[i]
        vals.append(np.sum(temp))
        
    del temp
    
    desc = [
        'All',
        'nJets',
        'nIsoLeps',
        'DiLepMDif',
        
        'DiJetMDif1',
        'DiJetMWdw1',
        'DiJetMDif2',
        'DiJetMWdw2',
        
        'MissPTWdw',
        'Thrust',
        'Evis',
        'MinHHMass',
        'nBJets',
    ]
    
    # Plot consecutive (&) cuts
    ax1 = sns.barplot(x=bars, y=vals, )
    
    ax1.set_yscale('log')
    ax1.set_ylabel('Number of events')
    ax1.set_xlabel('Preselection cut')
    ax1.set_title(rf'$2f$ events surviving cats (consecutive)')
    ax1.set_xticklabels(desc, rotation=45)
    
    # Plot individual cuts
    plt.show()
    
    ax2 = sns.barplot(x=bars, y=[ len(vecs), *[ np.sum(vecs.T[i]) for i in range(len(vecs.T)) ]])
    ax2.set_yscale('log')
    ax2.set_ylabel('Number of events')
    ax2.set_xlabel('Preselection cut')
    ax2.set_title(rf'$2f$ events surviving cats (individual)')
    ax2.set_xticklabels(desc, rotation=45)
    
    return [
        ax1.get_figure(),
        ax2.get_figure()
    ]