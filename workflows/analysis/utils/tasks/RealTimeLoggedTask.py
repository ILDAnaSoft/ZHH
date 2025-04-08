import datetime, json, os
from typing import cast
from .BaseTask import BaseTask

class RealTimeLoggedTask(BaseTask):
    def log_msg(self, msg:list|str|dict|float|int):
        os.makedirs(cast(str, self.htcondor_output_directory().path), exist_ok=True)
        
        with open(f'{self.htcondor_output_directory().path}/{self.branch}.txt', 'a+') as lf:
            if not isinstance(msg, list):
                msg = [msg]
            
            for msgg in msg:
                if not isinstance(msgg, str) or isinstance(msgg, float) or isinstance(msgg, int):
                    msgg = json.dumps(msgg)
                
                lf.write(f'{datetime.datetime.now().strftime("%d.%b %Y %H:%M:%S")}: {msgg}\n')