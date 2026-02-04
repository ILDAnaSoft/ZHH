from glob import glob

files = []

for dir in ['h_bb', 'h_cc', 'h_dd', 'h_gg', 'h_ss', 'h_uu']:
    files += glob(f'/group/ilc/tianjp/generator/PostDBD/whizard3/FlavorTag/n23n23h.eL.pR/{dir}/*.slcio')

files.sort()

with open('files.txt', 'w') as f:
    f.write('\n'.join(files))