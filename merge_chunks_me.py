import sys
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
            print("skip mst")
            continue
        merge_incomplete(p, meta)

    print(faceMaps)
    for k in faceMaps:
        merge_face(p,k,faceMaps[k])

def merge_chunks(p):
    #cu.merge_intermediate_outputs(p, "complete_edges")
    cu.merge_intermediate_outputs(p, "residual_rg")
    for meta in sys.argv[2:]:
        cu.merge_intermediate_outputs(p, "ongoing_{}".format(meta))
    d = p["children"]
    face_maps = {i : [d[k] for k in cu.generate_subface_keys(i) if k in d] for i in range(6)}
    merge_faces(p,face_maps)

param = cu.read_inputs(sys.argv[1])

if param["mip_level"] == 0:
    print("atomic chunk, nothing to merge")
else:
    print("mip level:", param["mip_level"])
    merge_chunks(param)
