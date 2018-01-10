import sys
import numpy as np
import chunk_utils as cu
from multiprocessing.dummy import Pool as ThreadPool

def read_seg_pairs(fn):
    with open(fn) as f:
        return set([tuple(l.split()) for l in f])

def read_seg_ids(fn):
    ids = np.fromfile(fn,dtype="uint64")
    return set(ids)

def load_incomplete_edges(p):
    d = p["children"]
    mip_c = p["mip_level"]-1
    prefix = "incomplete_edges_"
    incomplete_edges = {}
    for k in d:
        tag = cu.chunk_tag(mip_c, d[k])
        fn = prefix+tag+".data"
        incomplete_edges[tag] = read_seg_pairs(fn)
    return incomplete_edges

def merge_edge(dirname, e, incomplete_edges):
    fnList = []
    edge_name = "_".join(e)+".data"
    for k in incomplete_edges:
        if e in incomplete_edges[k]:
            fnList.append(k+"/"+edge_name)

    #merge_files(dirname+"/"+edge_name, fnList)
    return (dirname+"/"+edge_name, fnList)

def merge_and_process_edges(p, incomplete_edges, frozen_segs):
    target_edges = set()
    for k in incomplete_edges:
        target_edges |= incomplete_edges[k]

    incomplete_edges_list = open("incomplete_edges_"+cu.chunk_tag(p["mip_level"], p["indices"])+".data","w")
    process_edges_list = open("process_edges.data","w")

    incomplete_edges_dirname = cu.chunk_tag(p["mip_level"], p["indices"])
    process_edges_dirname = "edges"
    incomplete_fl = []
    process_fl = []
    pool = ThreadPool(16)
    for e in target_edges:
        if int(e[0]) in frozen_segs and int(e[1]) in frozen_segs:
            incomplete_edges_list.write("%s %s\n"%e)
            incomplete_fl.append(merge_edge(incomplete_edges_dirname, e, incomplete_edges))
        else:
            process_edges_list.write("%s %s\n"%e)
            process_fl.append(merge_edge(process_edges_dirname, e, incomplete_edges))

    pool.starmap(cu.merge_files, process_fl)
    pool.starmap(cu.merge_files, incomplete_fl)

def merge_face(p,idx,subFaces):
    mip_c = p["mip_level"]-1
    prefix = "boundary_"
    frozen_segs = set()
    for f in subFaces:
        fn = prefix+str(idx)+"_"+cu.chunk_tag(mip_c,f)+".data"
        frozen_segs |= read_seg_ids(fn)

    seg_array = np.array(list(frozen_segs))
    seg_array.tofile(prefix+str(idx)+"_"+cu.chunk_tag(p["mip_level"],p["indices"])+".data")

    return frozen_segs

def merge_faces(p, faceMaps):
    incomplete_edges = load_incomplete_edges(p)
    print(faceMaps)
    frozen_segs = set()
    for k in faceMaps:
        frozen_segs |= merge_face(p,k,faceMaps[k])

    merge_and_process_edges(p, incomplete_edges, frozen_segs)

def merge_chunks(p):
    cu.merge_intermediate_outputs(p, "residual_rg")
    d = p["children"]
    face_maps = {i : [d[k] for k in cu.generate_subface_keys(i) if k in d] for i in range(6)}
    merge_faces(p,face_maps)

param = cu.read_inputs(sys.argv[1])

if param["mip_level"] == 0:
    print("atomic chunk, nothing to merge")
else:
    print("mip level:", param["mip_level"])
    merge_chunks(param)
