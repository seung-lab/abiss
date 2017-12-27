import json
import sys
import shutil
import os
from multiprocessing.dummy import Pool as ThreadPool

def read_inputs(fn):
    with open(fn) as f:
        return json.load(f)

def chunk_tag(mip_level, indices):
    idx = [mip_level] + indices
    return "_".join([str(i) for i in idx])

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

def generate_subface_keys(idx):
    pos = idx % 3
    offset = idx // 3
    faces = [[i,j] for i in range(2) for j in range(2)]
    list(map(lambda l: l.insert(pos, offset), faces))
    return ["_".join([str(i) for i in l]) for l in faces]

def merge_incomplete_edges(p):
    d = p["children"]
    mip_c = p["mip_level"]-1
    prefix = "incomplete_edges_"
    fn = [prefix+chunk_tag(mip_c, d[k])+".data" for k in d]
    merge_files(prefix+chunk_tag(p["mip_level"],p["indices"])+".tmp", fn)

def merge_edge(dirname, e, incomplete_edges):
    fnList = []
    edge_name = "_".join(e)+".data"
    for k in incomplete_edges:
        if e in incomplete_edges[k]:
            fnList.append(k+"/"+edge_name)

    #merge_files(dirname+"/"+edge_name, fnList)
    return (dirname+"/"+edge_name, fnList)

def merge_face(p,idx,subFaces):
    mip_c = p["mip_level"]-1
    prefix = "boundary_"
    fn = [prefix+str(idx)+"_"+chunk_tag(mip_c,f)+".data" for f in subFaces]

    output = prefix+str(idx)+"_"+chunk_tag(p["mip_level"],p["indices"])+".tmp"

    merge_files(output, fn)

def merge_faces(p, faceMaps):
    merge_incomplete_edges(p)
    print(faceMaps)
    for k in faceMaps:
        merge_face(p,k,faceMaps[k])

def merge_intermediate_outputs(p, prefix):
    d = p["children"]
    mip_c = p["mip_level"]-1
    inputs = [prefix+"_"+chunk_tag(mip_c, d[k])+".data" for k in d]
    output = prefix+"_"+chunk_tag(p["mip_level"], p["indices"])+".data"
    merge_files(output, inputs)

def merge_chunks(p):
    merge_intermediate_outputs(p, "complete_edges")
    merge_intermediate_outputs(p, "mst")
    merge_intermediate_outputs(p, "residual_rg")
    d = p["children"]
    face_maps = {i : [d[k] for k in generate_subface_keys(i) if k in d] for i in range(6)}
    merge_faces(p,face_maps)

param = read_inputs(sys.argv[1])

if param["mip_level"] == 0:
    print("atomic chunk, nothing to merge")
else:
    print("mip level:", param["mip_level"])
    merge_chunks(param)
