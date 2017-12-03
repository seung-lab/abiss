import json
import sys
import shutil

def read_inputs(fn):
    with open(fn) as f:
        return json.load(f)

def chunk_tag(mip_level, indices):
    idx = [mip_level] + indices
    return "_".join([str(i) for i in idx])

def read_seg_pairs(fn):
    with open(fn) as f:
        return set([tuple(l.split()) for l in f])

def read_seg_ids(fn):
    with open(fn) as f:
        return set([l.strip() for l in f])

def merge_files(target, fnList):
    with open(target,"wb") as outfile:
        for fn in fnList:
            try:
                shutil.copyfileobj(open(fn, 'rb'), outfile)
            except IOError:
                print(fn, " does not exist")
                pass

def generate_subface_keys(idx):
    pos = idx % 3
    offset = idx // 3
    faces = [[i,j] for i in range(2) for j in range(2)]
    list(map(lambda l: l.insert(pos, offset), faces))
    return ["_".join([str(i) for i in l]) for l in faces]

def load_incomplete_edges(p):
    d = p["children"]
    mip_c = p["mip_level"]-1
    prefix = "incomplete_edges_"
    incomplete_edges = {}
    for k in d:
        tag = chunk_tag(mip_c, d[k])
        fn = prefix+tag+".dat"
        incomplete_edges[tag] = read_seg_pairs(fn)
    return incomplete_edges

def merge_edge(dirname, e, incomplete_edges):
    fnList = []
    edge_name = "_".join(e)+".dat"
    for k in incomplete_edges:
        if e in incomplete_edges[k]:
            fnList.append(k+"/"+edge_name)

    merge_files(dirname+"/"+edge_name, fnList)

def merge_and_process_edges(p, incomplete_edges, frozen_segs):
    target_edges = set()
    for k in incomplete_edges:
        target_edges |= incomplete_edges[k]

    incomplete_edges_list = open("incomplete_edges_"+chunk_tag(p["mip_level"], p["indices"])+".dat","w")
    process_edges_list = open("process_edges.dat","w")

    incomplete_edges_dirname = chunk_tag(p["mip_level"], p["indices"])
    process_edges_dirname = "edges"
    for e in target_edges:
        if e[0] in frozen_segs and e[1] in frozen_segs:
            incomplete_edges_list.write("%s %s\n"%e)
            merge_edge(incomplete_edges_dirname, e, incomplete_edges)
        else:
            process_edges_list.write("%s %s\n"%e)
            merge_edge(process_edges_dirname, e, incomplete_edges)

def merge_face(p,idx,subFaces):
    mip_c = p["mip_level"]-1
    prefix = "boundary_"
    frozen_segs = set()
    for f in subFaces:
        fn = prefix+str(idx)+"_"+chunk_tag(mip_c,f)+".dat"
        frozen_segs |= read_seg_ids(fn)

    with open(prefix+str(idx)+"_"+chunk_tag(p["mip_level"],p["indices"])+".dat","w") as f:
        for s in frozen_segs:
            f.write(s)
            f.write("\n")

    return frozen_segs

def merge_faces(p, faceMaps):
    incomplete_edges = load_incomplete_edges(p)
    print(faceMaps)
    frozen_segs = set()
    for k in faceMaps:
        frozen_segs |= merge_face(p,k,faceMaps[k])

    merge_and_process_edges(p, incomplete_edges, frozen_segs)

def merge_intermediate_outputs(p, prefix):
    d = p["children"]
    mip_c = p["mip_level"]-1
    inputs = [prefix+"_"+chunk_tag(mip_c, d[k])+".dat" for k in d]
    output = prefix+"_"+chunk_tag(p["mip_level"], p["indices"])+".dat"
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
