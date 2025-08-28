import h5py 
import time

# see https://stackoverflow.com/questions/69067142/reading-an-hdf5-file-only-after-it-has-completely-finished-acquiring-data
def h5_open_wait(h5file, mode:str='r', max_wait:int|float=30, interval:int|float=0.1):
    waited = 0

    while True:
        try:
            h5f = h5py.File(h5file, mode)
            return h5f
                
        except FileNotFoundError:
            print('Error: HDF5 File not found')
            return None
        
        except OSError:   
            if waited < max_wait:
                print(f'Error: HDF5 File at <{h5file}> locked, trying again...')
                time.sleep(interval) 
                waited += interval  
            else:
                print(f'waited too long= {waited} secs')
                return None