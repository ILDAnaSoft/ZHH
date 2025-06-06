import numpy as np
import luigi
import law
import os.path as osp
from glob import glob
from .utils import ForcibleTask, ShellTask, BaseTask

class SetupPackages(ShellTask, ForcibleTask, law.LocalWorkflow):
    packages = ['AddNeutralPFOCovMat', 'CheatedMCOverlayRemoval', 'LeptonPairing', 'HdecayMode', 'PreSelection', 'FinalStateRecorder']
    
    def output(self):
        return self.local_target('build_complete')
    
    def create_branch_map(self) -> dict[int, str]:
        return { k: self.packages[k] for k in range(len(self.packages)) }
    
    def complete(self):
        package = self.branch_data
        return (not self.force) and (osp.isfile(f'$REPO_ROOT/source/{package}/lib/lib{package}.so'))
    
    def build_command(self, fallback_level):
        package = self.branch_data
        
        cmd = f"""(source $REPO_ROOT/setup.sh &&{f' rm -rf "$REPO_ROOT/source/{package}/build" && rm -rf "$REPO_ROOT/source/{package}/lib" &&' if self.force else ''} mkdir -p "$REPO_ROOT/source/{package}/build" && cd "$REPO_ROOT/source/{package}/build" && ( if [ ! -f "$REPO_ROOT/source/{package}/lib/lib{package}.so" ]; then cmake .. && make install; fi ) && touch "{self.output().path}" ) || exit 11"""

        return cmd

# /pnfs/desy.de/ilc/prod/ilc/mc-opt-3/ild/dst-merged/500-TDR_ws/

class CheckDirectories(BaseTask):
    debug = luigi.BoolParameter(default=False)
    type = luigi.ChoiceParameter(choices=['dst-merged', 'sim', 'rec'], default='dst-merged')
    root_dir = luigi.Parameter(default='mc-opt-3')
    
    def search_masks(self):
        return [
            f'/pnfs/desy.de/ilc/prod/ilc/{self.root_dir}/ild/{self.type}/500-TDR_ws/*/ILD_l5_*/v02-*/*.slcio',
            f'/pnfs/desy.de/ilc/prod/ilc/{self.root_dir}/ild/{self.type}/500-TDR_ws/*/ILD_l5_*/v02-*/*/*.slcio'
        ]
    
    def output(self):
        return self.local_target('report.txt')
    
    def run(self):
        search_masks = self.search_masks()
        paths = glob(search_masks[0]) + glob(search_masks[1])
        paths.sort()
        
        if self.debug:
            paths = paths[:10]
        
        result = ''
        for path in paths:
            status = 'HEALTHY'
            
            try: # Try reading the first 100 bytes
                with open(path, 'r') as f:
                    f.read(100)
            except OSError as e:
                status = 'MISSING'
            except Exception as e:
                status = 'UNKNOWN'
            
            result += f'{status},{path}\n'
            
        self.output().dump(result)