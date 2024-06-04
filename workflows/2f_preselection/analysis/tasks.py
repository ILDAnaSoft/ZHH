# coding: utf-8

from zhh import get_raw_files
import law

# import our "framework" tasks
from analysis.framework import HTCondorWorkflow
from phc.tasks import ShellTask

class Preselection(ShellTask, HTCondorWorkflow, law.LocalWorkflow):
    """
    Simple task that has a trivial payload: converting integers into ascii characters. The task is
    designed to be a workflow with 26 branches. Each branch creates one character (a-z) and saves
    it to a json output file. While branches are numbered continuously from 0 to n-1, the actual
    data it processes is defined in the *branch_map*. A task can access this data via
    ``self.branch_map[self.branch]``, or via ``self.branch_data`` by convenience.

    By default, CreateChars is a HTCondorWorkflow (first workflow class in the inheritance order,
    MRO). If you want to execute it as a LocalWorkflow, add the ``"--workflow local"`` parameter on
    the command line. The code in this task should be completely independent of the actual *run
    location*, and law provides the means to do so.

    When a branch greater or equal to zero is set, e.g. via ``"--branch 1"``, you instantiate a
    single *branch task* rather than the workflow. Branch tasks are always executed locally.
    """

    def create_branch_map(self) -> dict[int, str]:
        # map branch indexes to ascii numbers from 97 to 122 ("a" to "z")
        arr = get_raw_files()
        res = { k: v for k, v in zip(list(range(len(arr))), arr) }
        
        return res #{i: num for i, num in enumerate(range(97, 122 + 1))}

    def output(self):
        # it's best practice to encode the branch number into the output target
        return self.local_target("output_{}.root".format(self.branch))

    def build_command(self, fallback_level):
        cmd =  f'source /afs/desy.de/user/b/bliewert/public/MarlinWorkdirs/ZHH/setup.sh'
        cmd += f'&& mkdir output'
        cmd += f'&& Marlin $REPO_ROOT/scripts/newZHHllbbbb.xml'
        cmd += f'&& mv output/* {self.output().path}'

        return cmd

