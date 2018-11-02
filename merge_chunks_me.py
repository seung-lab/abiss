import sys
import os
import chunk_utils as cu

def merge_incomplete(p, typ):
    d = p["children"]
    mip_c = p["mip_level"]-1
    prefix = "incomplete_{}_".format(typ)
    fn = [prefix+cu.chunk_tag(mip_c, d[k])+".data" for k in d]
    cu.merge_files(prefix+cu.chunk_tag(p["mip_level"],p["indices"])+".tmp", fn)

def merge_face(p,idx,subFaces):
    mip_c = p["mip_level"]-1
    prefix = "boundary_"
    fn = [prefix+str(idx)+"_"+cu.chunk_tag(mip_c,f)+".data" for f in subFaces]

    output = prefix+str(idx)+"_"+cu.chunk_tag(p["mip_level"],p["indices"])+".tmp"

    cu.merge_files(output, fn)

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

def merge_chunks(p):
    #cu.merge_intermediate_outputs(p, "complete_edges")
    cu.merge_intermediate_outputs(p, "residual_rg")
    cu.merge_intermediate_outputs(p, "ongoing")
    cu.merge_intermediate_outputs(p, "ongoing_supervoxel_counts")
    for meta in sys.argv[2:]:
        cu.merge_intermediate_outputs(p, "ongoing_{}".format(meta))
    d = p["children"]
    face_maps = {i : [d[k] for k in cu.generate_subface_keys(i) if k in d] for i in range(6)}
    merge_faces(p,face_maps)

def touch_done_files(f, tag):
    d = cu.generate_descedants(f,target=0)
    path = os.path.dirname(f)
    for c in d:
        cp = cu.read_inputs(os.path.join(path,c+".json"))
        fn_done = "remap/done_{}_{}.data".format(tag, cp["offset"])
        open(fn_done,'a').close()


param = cu.read_inputs(sys.argv[1])
ac_offset = param["ac_offset"]

if param["mip_level"] == 0:
    print("atomic chunk, nothing to merge")
else:
    print("mip level:", param["mip_level"])
    merge_chunks(param)
    touch_done_files(sys.argv[1], cu.chunk_tag(param["mip_level"], param["indices"]))

    with open("chunk_offset.txt", "w") as f:
        f.write(str(ac_offset))
