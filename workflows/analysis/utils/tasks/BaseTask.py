import luigi
import law
import os
from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from analysis.configurations import AnalysisConfiguration

class BaseTask(law.Task):
    tag = luigi.Parameter(
        default='500-all-full',
        description='Configuration set to run. Check configurations.py')
    
    # Custom postifx to be appended by inheriting tasks
    postfix:str = ''
    
    @property
    def config(self)->'AnalysisConfiguration':
        from analysis.framework import zhh_configs
        return zhh_configs.get(str(self.tag))
    
    @staticmethod
    def touch_parent(target:law.LocalTarget):
        if target.parent is not None:
            target.parent.touch()

    def local_path(self, *path):
        # DATA_PATH is defined in setup.sh
        parts = ("$DATA_PATH", )
        parts += (self.__class__.__name__ ,)
        parts += (str(self.tag) + self.postfix,)
        parts += path
        
        return os.path.join(*map(str, parts))

    def local_target(self, *path, format=None):
        return law.LocalFileTarget(self.local_path(*path), format=format)
    
    def local_directory_target(self, *path):
        return law.LocalDirectoryTarget(self.local_path(*path))
    
    def target_collection(self, targets):
        return law.TargetCollection(targets)