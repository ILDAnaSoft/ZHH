import pandas as pd

def df_append(df:pd.DataFrame, d:dict):    
    return pd.concat([df, pd.DataFrame(d)], ignore_index=True)
    