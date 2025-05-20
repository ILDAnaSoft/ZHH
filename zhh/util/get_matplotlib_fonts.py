import matplotlib
import matplotlib.font_manager

font_cache = {
    'fallback_font': 'DejaVu Sans',
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

def resolve_fonts(fonts:list[str]|str|None, fallback_font=font_cache['fallback_font']):
    if not len(font_cache['list']):
        font_cache['list'] = get_matplotlib_fonts()
        
    if fonts is not None:
        for font in (fonts if isinstance(fonts, list) else [fonts]):
            if font in font_cache['list']:
                return font

    assert(fallback_font in font_cache['list'])
    return fallback_font