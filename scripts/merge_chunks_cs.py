import sys
import struct
import chunk_utils as cu
import os

def merge_outer_face(p,idx,subFaces):
    prefixes = ["bc_{}_{}".format(i,idx) for i in range(3)]

    for prefix in prefixes:
        mip_c = p["mip_level"]-1
        print(prefix, subFaces)
        inputs = [prefix+"_"+cu.chunk_tag(mip_c, k)+".data" for k in subFaces]
        output = prefix+"_"+cu.chunk_tag(p["mip_level"], p["indices"])+".data"
        cu.merge_files(output, inputs)

def merge_overlapping_faces(p, f_idx, f_faces, b_faces):
    b_idx = f_idx-3
    f_prefixes = ["bc_{}_{}".format(i, f_idx) for i in range(3)]
    b_prefixes = ["bc_{}_{}".format(i, b_idx) for i in range(3)]

    f_list = []
    for prefixes, faces in zip([f_prefixes,b_prefixes],[f_faces, b_faces]):
        for prefix in prefixes:
            mip_c = p["mip_level"]-1
            inputs = [prefix+"_"+cu.chunk_tag(mip_c, k)+".data" for k in faces]
            f_list.append(inputs)
    return f_list

def merge_faces(p, faceMaps):
    flags = p["boundary_flags"]
    for k in faceMaps:
        if (flags[k] == 0):
            #print k, faceMaps[k]
            merge_outer_face(p,k,faceMaps[k])

    input_files = [[] for i in range(6)]
    bbox = p["bbox"]
    dims = [bbox[i+3]-bbox[i] for i in range(3)]
    size = 0
    slice_size = [dims[1]*dims[2], dims[0]*dims[2], dims[0]*dims[1]]
    for i in range(3):
        if len(faceMaps[i+3]) == 0:
            continue
        size += slice_size[i]
        f_list = merge_overlapping_faces(p,i+3,faceMaps[i], faceMaps[i+3])
        for i in range(len(f_list)):
            input_files[i] += f_list[i]

    prefixes = ["bc_f_{}".format(i) for i in range(3)]+["bc_b_{}".format(i) for i in range(3)]
    for prefix, inputs in zip(prefixes, input_files):
        output = prefix+".data"
        print(output, inputs)
        cu.merge_files(output, inputs)
    return size

def write_param(p):
    flags = p["boundary_flags"]

    with open("param.txt","w") as f:
        f.write(" ".join([str(i) for i in flags]))

def merge_chunks(p):
    d = p["children"]
    cu.merge_intermediate_outputs(p, "incomplete_cs")

    face_maps = {i : [d[k] for k in cu.generate_subface_keys(i) if k in d] for i in range(6)}
    s = merge_faces(p,face_maps)
    write_param(p)

param = cu.read_inputs(sys.argv[1])

if param["mip_level"] == 0:
    print("atomic chunk, nothing to merge")
else:
    print("mip level:", param["mip_level"])
    merge_chunks(param)
