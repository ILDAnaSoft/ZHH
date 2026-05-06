import os.path as osp
from subprocess import check_output
from os import makedirs, getcwd, chdir, listdir
from glob import glob
from shutil import move
from time import sleep

def ctan_parse_package_meta(pkg_name:str)->dict:
    import requests, json

    request = requests.get(f'http://www.ctan.org/json/2.0/pkg/{pkg_name}')
    if request.status_code != 200:
        raise Exception(f'Could not find package <{pkg_name}> on CTAN')
    
    return json.loads(request.text)

def install_ctan_package(pkg_name:str, install_dir:str|None=None, req_headers:dict={}):
    import zipfile, requests

    if osp.isfile(f'{install_dir}/{pkg_name}.sty'):
        print(f'Found existing installation for package <{pkg_name}> at <{install_dir}/{pkg_name}>')
        return True
    
    if install_dir is None:
        install_dir = getcwd()

    if not 'User-Agent' in req_headers:
        req_headers['User-Agent'] = 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) '

    start_dir = getcwd()

    try:
        meta = ctan_parse_package_meta(pkg_name)
        if not 'ctan' in meta or not meta['ctan'].get('file', False):
            raise Exception(f'No file to download for package <{pkg_name}>')

        try:
            # get files to download
            if False:
                url = f"https://mirrors.ctan.org{meta['ctan']['path']}"
                print(url)

                dir_req = requests.get(url)
                assert(dir_req.status_code == 200)

                # get files to download
                entries = dir_req.text.split('href="')[2:]
                files = []

                for entry in [ match.split('</a>')[0] for match in entries ]:
                    items = entry.split('">')
                    if len(items) != 2 or './' in items[0] or items[0] != items[1]:
                        continue

                    files.append(items[0])

                print(f'Getting {len(files)} files for package <{pkg_name}>', files)

            url = f"https://ctan.org/tex-archive{meta['ctan']['path']}"
            dir_req = requests.get(url, headers=req_headers)
            assert(dir_req.status_code == 200)

            zip_url = dir_req.text.split('class="download">')[1]
            zip_url = zip_url.split('href="')[1].split('">')[0]
            
            assert(zip_url.endswith('.zip'))
            fname = zip_url.split('/')[-1]

            r = requests.get(zip_url, stream=True)
            with open(f'{install_dir}/{fname}', 'wb') as f:
                for chunk in r.iter_content(chunk_size=1024): 
                    if chunk:
                        f.write(chunk)

            assert(r.status_code == 200)
        except Exception as e:
            print(e)
            raise Exception(f'Could not download package <{pkg_name}>')

        zip_dir = f'{install_dir}/{osp.splitext(fname)[0]}'
        try:
            with zipfile.ZipFile(f'{install_dir}/{fname}', 'r') as zip_ref:
                zip_ref.extractall(install_dir)

            assert(osp.isdir(zip_dir))
            sleep(1)
        except Exception as e:
            print(e)
            raise Exception(f'Could not unzip file from <{install_dir}/{fname}> to dir <{zip_dir}>')
        
        try:
            for file in listdir(zip_dir):
                move(f'{zip_dir}/{file}', f'{install_dir}/{osp.basename(file)}')

            chdir(install_dir)

            if osp.isfile(f'{pkg_name}.ins'):
                check_output([f'tex {pkg_name}.ins'], shell=True)
            elif osp.isfile(f'{pkg_name}.dtx'):
                check_output([f'tex {pkg_name}.dtx'], shell=True)
            else:
                raise Exception(f'Neither {pkg_name}.ins nor {pkg_name}.dtx found')
        except Exception as e:
            print(e)
            raise Exception(f'Could not install package <{pkg_name}> from .ins or .dtx file')
    finally:
        chdir(start_dir)
    
    print(f'Installed package <{pkg_name}>')

class LatexRenderContext:
    def __init__(self, work_dir:str|None=None, packages:list[str]=[]):
        """Helper to build latex documents with rudimentary package support using CTAN.
        Beware of the ancient TeX-live versions on DESY NAF, tho. siunitx may, among
        others, not work with "newer" features.

        Args:
            packages (list[str]): _description_
            work_dir (str | None, optional): _description_. Defaults to None.
        """
        self._packages = packages
        self._work_dir = work_dir if work_dir is not None else getcwd()
        self._ready = False

    def render(self, latex:str, location:str, overwrite:bool=True, preamble:str|None=None, documentclass:str='[border=8pt]{standalone}', timeout:float|int=60):
        if preamble is None:
            preamble = default_latex_preamble()

        makedirs(self._work_dir, exist_ok=True)

        content = "\documentclass" + documentclass + "\n\\usepackage{amsmath,amssymb}\n\\usepackage{booktabs}\n" + preamble + "\n\\begin{document}\n "
        content+= latex + " \n\\end{document}"

        bname = osp.splitext(osp.basename(location))[0]
        
        makedirs(self._work_dir, exist_ok=True)

        if not self._ready:
            for package in self._packages:
                install_ctan_package(package, install_dir=self._work_dir)
            
            self._ready = True
        
        tmp_file = f'{self._work_dir}/{bname}.tex'

        with open(tmp_file, 'w') as f:
            f.write(content)

        cmd = ''
        if overwrite:
            cmd += f'rm -f "{self._work_dir}/{bname}.pdf" && '
            
        cmd += f'pdflatex -output-directory "{self._work_dir}" "{tmp_file}"'        
        cmd += f' && cp "{self._work_dir}/{bname}.pdf" "{location}"'# && rm -rf "{tmp_dir}"'

        try:
            check_output([cmd], shell=True, timeout=timeout)
        except Exception as e:
            print(e)
            log_file = f'{osp.splitext(tmp_file)[0]}'
            if osp.isfile(log_file):
                print(f'---------- PDFLATEX LOG START [{log_file}] ----------')
                with open(log_file, 'tr') as tf:
                    print(tf.read())
                print(f'---------- PDFLATEX LOG END [{log_file}] ----------')

            raise Exception(f'Timeout after {timeout} seconds. Please check that the latex input is valid.')

def render_latex(latex:str, location:str, overwrite:bool=True, preamble:str|None=None, documentclass:str='[9pt]{standalone}', packages:list[str]=[]):
    drn = osp.dirname(location)
    bname = osp.splitext(osp.basename(location))[0]
    
    tmp_dir = f'{drn}/latex-build-{bname}'

    render_context = LatexRenderContext(work_dir=tmp_dir, packages=packages)
    render_context.render(latex, location, overwrite, preamble, documentclass)

def default_latex_preamble()->str:
    with open(osp.expandvars('$REPO_ROOT/zhh/resources/CutflowTableLatexPreamble.tex')) as tf:
        return tf.read()