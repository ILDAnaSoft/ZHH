def is_readable(path:str)->bool:
    try: # Try reading the first 100 bytes
        with open(path, mode='rb') as f:
            f.read(100)
            
        return True
    except Exception as e:
        print(e)
        return False