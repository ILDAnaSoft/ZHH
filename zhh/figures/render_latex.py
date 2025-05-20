import os.path as osp
from subprocess import check_output
from os import makedirs

def render_latex(latex:str, location:str, overwrite:bool=True):    
    content = "\documentclass[9pt]{article}\n\\usepackage[a4paper,margin=0.5cm,nohead,landscape]{geometry}\n\\usepackage{amsmath,amssymb}\n\\begin{document}\n " + latex + " \n\\end{document}"

    drn = osp.dirname(location)
    bname = osp.splitext(osp.basename(location))[0]
    
    tmp_dir = f'{drn}/TMP-{bname}'
    makedirs(tmp_dir, exist_ok=True)
    
    tmp_file = f'{tmp_dir}/{bname}.tex'

    with open(tmp_file, 'w') as f:
        f.write(content)
        
    cmd = f'pdflatex -output-directory "{tmp_dir}" "{tmp_file}"'
    if overwrite:
        cmd += f' && rm -f "{drn}/{bname}.pdf"'
    
    cmd += f' && mv "{tmp_dir}/{bname}.pdf" "{drn}" && rm -rf "{tmp_dir}"'
    check_output([cmd], shell=True)