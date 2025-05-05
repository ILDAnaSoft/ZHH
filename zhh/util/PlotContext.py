from matplotlib.colors import ListedColormap, Colormap
from typing import cast, Any

class PlotContext:
    def __init__(self, cmap:ListedColormap|Any) -> None:
        self._cmap = cmap
        self._keys = []
        self._keys_idx_map = {}
    
    def getColorByKey(self, key):
        if not key in self._keys_idx_map:
            self._keys_idx_map[key] = len(self._keys)
            self._keys.append(key)
        
        return cast(list, self._cmap.colors)[self._keys_idx_map[key]]
    
    def getColorPalette(self, keys:list):
        return [self.getColorByKey(key) for key in keys]