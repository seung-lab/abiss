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

def generate_ancestors(f, target=None):
    p = read_inputs(f)
    top_mip = p["top_mip_level"]
    mip = p["mip_level"]
    indices = p["indices"]
    ancestor = []
    while mip < top_mip:
        mip += 1
        indices = parent(indices)
        if target is None or mip == target:
            ancestor.append(chunk_tag(mip, indices))

    return ancestor

def generate_siblings(f):
    param = read_inputs(f)
    indices = param["indices"]
    mip = param["mip_level"]
    boundary_flags = param["boundary_flags"]

    siblings = [indices]
    for i, f in enumerate(boundary_flags[3:]):
        if f == 0:
            extra=[]
            for s in siblings:
                c = s[:]
                c[i] += 1
                extra.append(c)
            siblings+=extra

    return [chunk_tag(mip, s) for s in siblings]

def generate_descedants(f, target=None):
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
            if target is None or mip_c == target:
                descedants.append(tag)

        for k in d:
            tag = chunk_tag(mip_c, d[k])
            f_c = os.path.join(path, tag+".json")
            descedants += generate_descedants(f_c, target)

        return descedants

