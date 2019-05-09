import sys
import struct
import chunk_utils as cu
import os

def merge_outer_face(p,idx,subFaces):
    prefixes = ["seg_i_{}".format(idx), "seg_o_{}".format(idx)]
    if idx < 3:
        prefixes.append("aff_i_{}".format(idx))

    for prefix in prefixes:
        mip_c = p["mip_level"]-1
        print(prefix, subFaces)
        inputs = [prefix+"_"+cu.chunk_tag(mip_c, k)+".data" for k in subFaces]
        output = prefix+"_"+cu.chunk_tag(p["mip_level"], p["indices"])+".data"
        cu.merge_files(output, inputs)

def merge_overlapping_faces(p, f_idx, f_faces, b_faces):
    b_idx = f_idx-3
    f_prefixes = ["seg_i_{}".format(f_idx), "seg_o_{}".format(f_idx)]
    b_prefixes = ["seg_i_{}".format(b_idx), "seg_o_{}".format(b_idx), "aff_i_{}".format(b_idx)]

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

    input_files = [[] for i in range(5)]
    bbox = p["bbox"]
    dims = [bbox[i+3]-bbox[i] for i in range(3)]
    size = 0
    slice_size = [dims[1]*dims[2], dims[0]*dims[2], dims[0]*dims[1]]
    for i in range(3):
        if len(faceMaps[i+3]) == 0:
            continue
        size += slice_size[i]
        f_list = merge_overlapping_faces(p,i+3,faceMaps[i], faceMaps[i+3])
        for i in range(5):
            input_files[i] += f_list[i]

    prefixes = ["seg_fi", "seg_fo", "seg_bi", "seg_bo", "aff_b"]
    for prefix, inputs in zip(prefixes, input_files):
        output = prefix+".data"
        print(output, inputs)
        cu.merge_files(output, inputs)
    return size

def write_param(p, face_size):
    prefix = "meta"
    d = p["children"]
    mip_c = p["mip_level"]-1
    inputs = [prefix+"_"+cu.chunk_tag(mip_c, d[k])+".data" for k in d]
    counts = 0
    dend = 0
    bbox = p["bbox"]
    flags = p["boundary_flags"]
    sizes = [bbox[i+3]-bbox[i] for i in range(3)]
    remaps = 0
    ac_offset = p["ac_offset"]
    if len(inputs) > 1:
        remaps = os.stat("ongoing.data").st_size//16
        for f in inputs:
            data = open(f, "rb").read()
            meta_data = struct.unpack("llllll", data)
            counts += meta_data[3]
            dend += meta_data[4]


    with open("param.txt","w") as f:
        f.write(" ".join([str(i) for i in sizes]))
        f.write("\n")
        f.write(" ".join([str(i) for i in flags]))
        f.write("\n")
        f.write("{} {} {} {} {}".format(face_size, counts, dend, remaps, ac_offset))

def merge_chunks(p):
    d = p["children"]
    if (len(d) == 1):
        cu.lift_intermediate_outputs(p, "dend")
        cu.lift_intermediate_outputs(p, "counts")
        cu.lift_intermediate_outputs(p, "meta")
        cu.lift_intermediate_outputs(p, "ongoing")

        f = "meta_"+cu.chunk_tag(p["mip_level"], p["indices"])+".data"
        data = open(f, "rb").read()
        meta_data = list(struct.unpack("llllll", data))
        meta_data[5] = 0
        open(f,"wb").write(struct.pack("llllll", *meta_data))

        f = "remap_"+cu.chunk_tag(p["mip_level"], p["indices"])+".data"
        open(f, 'a').close()

    else:
        cu.merge_intermediate_outputs(p, "dend")
        cu.merge_intermediate_outputs(p, "counts")
        cu.merge_intermediate_outputs(p, "ongoing")

    face_maps = {i : [d[k] for k in cu.generate_subface_keys(i) if k in d] for i in range(6)}
    s = merge_faces(p,face_maps)
    write_param(p,s)

param = cu.read_inputs(sys.argv[1])

if param["mip_level"] == 0:
    print("atomic chunk, nothing to merge")
else:
    print("mip level:", param["mip_level"])
    merge_chunks(param)

    cu.touch_done_files(sys.argv[1], cu.chunk_tag(param["mip_level"], param["indices"]))
