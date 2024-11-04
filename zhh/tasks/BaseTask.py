import luigi
import law
import os

class BaseTask(law.Task):
    tag = luigi.Parameter(default='500-all-full')
    
    # Custom postifx to be appended by inheriting tasks
    postfix:str = ''

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