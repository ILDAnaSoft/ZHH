from tabulate import tabulate, SEPARATING_LINE
import shutil
import os.path as osp
from os import makedirs
from subprocess import check_output

latex_replacements = [
    ('>=', r'\geq '),
    ('<=', r'\leq '),
    ('->', r'\rightarrow '),
    ('<-', r'\leftarrow '),
    ('=>', r'\Rightarrow '),
    ('<=', r'\Leftarrow '),
]
    
def render_table(lines:list[list[str]|str], col_sep:str='8pt', row_sep:float=1.2, replacements:list[tuple[str, str]]|None=latex_replacements):            
    column_widths = [1] * len(lines[0])
    n_lines = len(lines)
    
    for line in lines:
        if not isinstance(line, str):
            for i, col in enumerate(line):
                if isinstance(col, str):
                    column_widths[i] = max(column_widths[i], len(col))
    
    content = ""
    for i in range(n_lines):
        #print(lines[i], isinstance(lines[i], str))
        if isinstance(lines[i], str):
            #print('hi')
            content += rf'{lines[i]}' + ' \n'
        else:
            content += f'{" & ".join([str(x).ljust(column_widths[j], " ") for j, x in enumerate(lines[i])])} \\\\ \n'
    
    if replacements is not None:
        for old, new in replacements:
            content = content.replace(old, new)
    
    return rf"""\setlength{{\tabcolsep}}{{ {col_sep} }}
\renewcommand{{\arraystretch}}{{ {row_sep} }}
\begin{{tabular}}{{ {'r' * len(lines[0])} }}
\hline{content}\hline
\end{{tabular}}"""