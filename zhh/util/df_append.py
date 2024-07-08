import pandas as pd

def df_append(df:pd.DataFrame, d:dict):
    dict_df = pd.DataFrame({
        'name': d.keys(),
        'value': d.values()
    })
    
    return pd.concat([df, dict_df], ignore_index=True)
    