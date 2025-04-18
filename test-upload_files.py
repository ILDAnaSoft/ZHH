# requires pip install git+ssh://git@github.com/nVentis/pyocclient

import owncloud, hashlib, argparse
import os.path as osp
from hashlib import md5
from tqdm.auto import tqdm
from glob import glob

public_link = 'https://syncandshare.desy.de/index.php/s/4scFfaJCScXDweX'

if __name__=="__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--filelist', type=str, required=False, help='path to a newline separated list of files')
    parser.add_argument('--files', type=str, required=False, help='comma-separated list of files')
    parser.add_argument('--file', type=str, required=False, help='path to a file')
    
    args = parser.parse_args()
    
    filelist = str(args.filelist)
    files = str(args.files)
    file = str(args.file)
    
    YOUR_FILES = []
    if filelist != 'None':
        with open(filelist, 'r') as fh:
            YOUR_FILES = fh.read().split('\n')
    elif files != 'None':
        YOUR_FILES = files.split(',')
    elif file != 'None':
        YOUR_FILES.append(file)
    else:
        raise Exception('Either filelist, files or file must be given')

    oc = owncloud.Client.from_public_link(public_link)

    for file in (pbar := tqdm(YOUR_FILES)):
        bname = osp.basename(file)
        pbar.set_description(f'Uploading file {bname}')
        
        try:
            oc.file_info(bname)
            pbar.set_description('File exists...')
        except owncloud.ResponseError as e:
            if e.status_code == 404:
                # manual chunking
                result = oc.drop_file(file)
                if result['success'] and result['chunk_count'] > 1:
                    oc.put_file_contents(bname, f'<CHUNK_COUNT:TRANSFER_ID>={result["chunk_count"]}:{result["transfer_id"]}')
            else:
                print(f'Warning: Unhandled error while attempting to upload <{file}>', e)