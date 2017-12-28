import json
import os
import shutil

def read_inputs(fn):
    with open(fn) as f:
        return json.load(f)

def chunk_tag(mip_level, indices):
    idx = [mip_level] + indices
    return "_".join([str(i) for i in idx])

def parent(indices):
    return [i//2 for i in indices]

def merge_files(target, fnList):
    if len(fnList) == 1:
        os.rename(fnList[0],target)
        return

    with open(target,"wb") as outfile:
        for fn in fnList:
            try:
                shutil.copyfileobj(open(fn, 'rb'), outfile)
                os.remove(fn)
            except IOError:
                print(fn, " does not exist")
                pass
