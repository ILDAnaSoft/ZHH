import matplotlib
import matplotlib.font_manager

font_cache = {
    'fallback_font': 'Arial',
    'list': []
}

def get_matplotlib_fonts()->list[str]:
    font_families = []
    fpaths = matplotlib.font_manager.findSystemFonts()

    for i in fpaths:
        if not 'emoji' in i.lower():
            f = matplotlib.font_manager.get_font(i)
            font_families.append(f.family_name)
            
    return font_families

def resolve_fonts(fonts:list[str], fallback_font=font_cache['fallback_font']):
    if not len(font_cache['list']):
        font_cache['list'] = get_matplotlib_fonts()
    
    for font in fonts:
        if font in font_cache['list']:
            return font

    return fallback_font