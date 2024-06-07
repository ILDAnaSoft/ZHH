import luigi
import law
import os.path as osp
from glob import glob
from phc import ForcibleTask, ShellTask, BaseTask

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

class CheckDirectories(BaseTask, law.LocalWorkflow):
    search_masks = [
        '/pnfs/desy.de/ilc/prod/ilc/mc-opt-3/ild/dst-merged/500-TDR_ws/*/ILD_l5_*/v02-*/*.slcio',
        '/pnfs/desy.de/ilc/prod/ilc/mc-opt-3/ild/dst-merged/500-TDR_ws/*/ILD_l5_*/v02-*/*/*.slcio'
    ]
    debug = luigi.BoolParameter(default=False)
    
    def output(self):
        return self.local_target('report.txt')
    
    def create_branch_map(self) -> dict[int, str]:
        paths = glob(self.search_masks[0]) + glob(self.search_masks[1])
        paths.sort()
        if self.debug:
            paths = paths[:10]
        
        paths = { k: paths[k] for k in range(len(paths)) }
        
        return paths
    
    def run(self):
        id = self.branch
        path = self.branch_data
        result = "HEALTHY"
        
        try:
            with open(self.branch_data, 'r') as f:
                f.read(100)
        except OSError as e:
            result = "MISSING"
        except Exception as e:
            result = "UNKNOWN"
        
        self.output().dump(f'{id},{result},{path}\n')