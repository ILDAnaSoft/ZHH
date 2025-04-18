#from phc import module_reload
#module_reload('zhh')

from zhh import calc_preselection_by_processes, apply_cuts, zhh_cuts
import numpy as np

from os.path import dirname, basename, splitext
from subprocess import run
from tabulate import tabulate, SEPARATING_LINE

def render_latex(latex:str, location:str):
    latex = latex.replace('_', '\\textunderscore ')
    latex = latex.replace('>=', '$\geq$')
    latex = latex.replace('<=', '$\leq$')
    
    content = "\documentclass[9pt]{article}\n\\usepackage[a4paper,margin=1in,landscape]{geometry}\n\\begin{document}\n " + latex + " \n\\end{document}"

    drn = dirname(location)
    se = splitext(basename(location))

    with open(f'{drn}/{se[0]}.tex', 'w') as f:
        f.write(content)
        
    run(['pdflatex', '-output-directory', drn, f'{drn}/{se[0]}.tex'])