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

def generate_subface_keys(idx):
    pos = idx % 3
    offset = idx // 3
    faces = [[i,j] for i in range(2) for j in range(2)]
    list(map(lambda l: l.insert(pos, offset), faces))
    return ["_".join([str(i) for i in l]) for l in faces]

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

def lift_intermediate_outputs(p, prefix):
    d = p["children"]
    mip_c = p["mip_level"]-1
    inputs = [prefix+"_"+chunk_tag(mip_c, d[k])+".data" for k in d]
    output = prefix+"_"+chunk_tag(p["mip_level"], p["indices"])+".data"
    merge_files(output, inputs)

def merge_intermediate_outputs(p, prefix):
    d = p["children"]
    mip_c = p["mip_level"]-1
    inputs = [prefix+"_"+chunk_tag(mip_c, d[k])+".data" for k in d]
    output = prefix+".data"
    merge_files(output, inputs)

def generate_ancestors(f):
    p = read_inputs(f)
    top_mip = p["top_mip_level"]
    mip = p["mip_level"]
    indices = p["indices"]
    ancestor = []
    while mip < top_mip:
        mip += 1
        indices = parent(indices)
        ancestor.append(chunk_tag(mip, indices))
    return ancestor

def generate_descedants(f):
    path = os.path.dirname(f)
    p = read_inputs(f)
    mip = p["mip_level"]
    if mip == 0:
        return []
    else:
        mip_c = mip - 1
        d = p["children"]
        descedants = []
        for k in d:
            tag = chunk_tag(mip_c, d[k])
            descedants.append(tag)
            f_c = tag+".json"
            if path:
                f_c = path+"/"+f_c
            descedants += generate_descedants(f_c)
        return descedants

