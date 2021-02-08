import sys
import os
import chunk_utils as cu

def merge_incomplete(p, typ):
    d = p["children"]
    mip_c = p["mip_level"]-1
    prefix= "incomplete_{}_".format(typ)
    fn = [prefix+cu.chunk_tag(mip_c, d[k])+".data" for k in d]
    prefix = "o_incomplete_{}_".format(typ) if os.environ['OVERLAP'] == '1' else "incomplete_{}_".format(typ)
    cu.merge_files(prefix+cu.chunk_tag(p["mip_level"],p["indices"])+".tmp", fn)

def merge_face(p,idx,subFaces):
    mip_c = p["mip_level"]-1

    prefix = "boundary_"

    fn = [prefix+str(idx)+"_"+cu.chunk_tag(mip_c,f)+".data" for f in subFaces]

    prefix = "o_boundary_" if os.environ['OVERLAP'] == '1' else "boundary_"

    output = prefix+str(idx)+"_"+cu.chunk_tag(p["mip_level"],p["indices"])+".tmp"

    cu.merge_files(output, fn)


def merge_vanished_faces(p, faces):
    mip_c = p["mip_level"]-1
    d = p["children"]
    prefix = "boundary_"
    fn = []
    for k in faces:
        if k in d:
            fn+=[prefix+str(i)+"_"+cu.chunk_tag(mip_c,d[k])+".data" for i in faces[k]]
    cu.merge_files("matching_faces.data", fn)


def merge_faces(p, faceMaps):
    merge_incomplete(p, "edges")

    for meta in sys.argv[2:]:
        if meta == "mst":
            print("skip mst, mst is either ongoing or complete, there is no incomplete ones")
            continue
        merge_incomplete(p, meta)

    print(faceMaps)
    for k in faceMaps:
        merge_face(p,k,faceMaps[k])

    vanished_faces = cu.generate_vanished_subface()
    merge_vanished_faces(p, vanished_faces)

def merge_chunks(p):
    #cu.merge_intermediate_outputs(p, "complete_edges")
    cu.merge_intermediate_outputs(p, "residual_rg")
    cu.merge_intermediate_outputs(p, "ongoing")
    cu.merge_intermediate_outputs(p, "ongoing_supervoxel_counts")
    cu.merge_intermediate_outputs(p, "ongoing_semantic_labels")
    cu.merge_intermediate_outputs(p, "ongoing_seg_size")
    if os.environ['OVERLAP'] == '1':
        cu.merge_intermediate_outputs(p, "vetoed_edges")

    for meta in sys.argv[2:]:
        cu.merge_intermediate_outputs(p, "ongoing_{}".format(meta))
    d = p["children"]
    face_maps = {i : [d[k] for k in cu.generate_subface_keys(i) if k in d] for i in range(6)}
    merge_faces(p,face_maps)


param = cu.read_inputs(sys.argv[1])
ac_offset = param["ac_offset"]

if param["mip_level"] == 0:
    print("atomic chunk, nothing to merge")
else:
    print("mip level:", param["mip_level"])
    merge_chunks(param)
    cu.touch_done_files(sys.argv[1], cu.chunk_tag(param["mip_level"], param["indices"]))

    with open("chunk_offset.txt", "w") as f:
        f.write(str(ac_offset))
