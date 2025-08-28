from matplotlib.colors import ListedColormap, Colormap
from typing import cast, Any

class PlotContext:
    def __init__(self, cmap:ListedColormap|Any=None, font:str|list[str]|None=None) -> None:
        if cmap is None:
            from zhh import colormap_desy
            cmap = colormap_desy
        
        if not isinstance(font, str):
            from zhh import resolve_fonts
            font = resolve_fonts(font)
        
        assert(isinstance(font, str))
        
        self._cmap = cmap
        self._font = font
        self._keys = []
        self._keys_idx_map = {}
    
    def getFont(self):
        return self._font
    
    def getColorByKey(self, key):
        if not key in self._keys_idx_map:
            self._keys_idx_map[key] = len(self._keys)
            if len(cast(list, self._cmap.colors)) == len(self._keys):
                raise Exception('The colormap does not contain any remaining entries')
                
            self._keys.append(key)
        
        return cast(list, self._cmap.colors)[self._keys_idx_map[key]]
    
    def getColorsAssignedKeys(self):
        return self._keys
    
    def getColorPalette(self, keys:list):
        return [self.getColorByKey(key) for key in keys]
    
    def __copy__(self):
        raise Exception('PlotContext instances are unique')

    def __deepcopy__(self, memo):
        raise Exception('PlotContext instances are unique')