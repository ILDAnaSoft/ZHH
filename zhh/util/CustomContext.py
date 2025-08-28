from collections.abc import Callable

class CustomContext:
    def __init__(self, enter:Callable, exit:Callable):
        self._enter = enter
        self._exit = exit
        
    def __enter__(self, *args, **kwargs):
        return self._enter(*args, **kwargs)
    
    def __exit__(self, exc_type, exc_val, traceback):
        return self._exit(exc_type, exc_val, traceback)