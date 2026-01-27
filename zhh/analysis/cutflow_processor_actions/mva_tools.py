import numpy as np

def get_signal_categories(signal_classes:list[int], classes:list[tuple[int, str]])->list[int]:
    from zhh import EventCategories
    indices = []

    for label, name in classes:
        try:
            if getattr(EventCategories, name) in signal_classes:
                indices.append(label)
        except AttributeError as e:
            print(f'Warning: Could not find class <{name}> with label <{label}>. Assuming it\'s background. If it is not, make sure to use a name that is registered in EventCategories.py!')

    return indices

def get_background_categories(signal_indices:list[int], classes:list[tuple[int, str]])->list[int]:
    all_indices = np.arange(len(classes), dtype=int)

    return list(all_indices[~np.isin(all_indices, signal_indices)])