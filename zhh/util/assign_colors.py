from typing import Iterable, List

def assign_colors(colorpalette_full:List, colorpalette_temp:dict, keys:Iterable)->List:
    """Compiles a list of colors for a given list of keys. If a key is not in 
    colorpalette_temp, a new color is assigned to it.

    Args:
        colorpalette_full (List): _description_
        colorpalette_temp (dict): _description_
        keys (_type_): _description_

    Returns:
        List: _description_
    """
    colors = []
    
    for key in keys:
        if key not in colorpalette_temp:
            new_color = colorpalette_full[len(colorpalette_temp.keys())]
            colorpalette_temp[key] = new_color
            colors.append(new_color)
        else:
            colors.append(colorpalette_temp[key])
            
    return colors