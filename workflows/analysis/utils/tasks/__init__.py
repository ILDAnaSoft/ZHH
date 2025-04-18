# From https://github.com/nVentis/pyhepcommon

from os import name

if name != 'nt':
    from .BaseTask import BaseTask
    from .ShellTask import ShellTask
    from .ForcibleTask import ForcibleTask
    from .RealTimeLoggedTask import RealTimeLoggedTask
    
del name