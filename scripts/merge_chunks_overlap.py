import sys
import os
import chunk_utils as cu

def merge_incomplete(p, typ):
    d = p["neighbours"]
    mip = p["mip_level"]
    prefix = "incomplete_{}_".format(typ)
    fn = [prefix+cu.chunk_tag(mip, d[k])+".data" for k in d]
    cu.merge_files(prefix+cu.chunk_tag(p["mip_level"],p["indices"])+".tmp", fn)

def merge_face(p,idx,subFaces):
    mip = p["mip_level"]
    prefix = "boundary_"
    fn = [prefix+str(idx)+"_"+cu.chunk_tag(mip,f)+".data" for f in subFaces]

    output = prefix+str(idx)+"_"+cu.chunk_tag(p["mip_level"],p["indices"])+".tmp"

    cu.merge_files(output, fn)

def merge_cut_plane(p, idx):
    mip = p["mip_level"]
    prefix = "boundary_"
    pos = idx % 3
    offset = 0
    faces = [[i,j] for i in range(-1,2,1) for j in range(-1,2,1)]
    list(map(lambda l: l.insert(pos, offset), faces))
    candidates = ["_".join([str(i) for i in l]) for l in faces]
    d = p["neighbours"]
    neighbours = [d[c] for c in candidates if c in d]
    fn = [prefix+str(idx)+"_"+cu.chunk_tag(mip,f)+".data" for f in neighbours]
    print("merge face {} with {}".format(idx, neighbours))

    output = "cut_plane_"+str(idx)+"_"+cu.chunk_tag(p["mip_level"],p["indices"])+".data"

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

def merge_neighbour_outputs(p, prefix):
    d = p["neighbours"]
    mip = p["mip_level"]
    inputs = [prefix+"_"+cu.chunk_tag(mip, d[k])+".data" for k in d]
    output = prefix+".data"
    cu.merge_files(output, inputs)


def merge_chunks(p):
    #cu.merge_intermediate_outputs(p, "complete_edges")
    merge_neighbour_outputs(p, "residual_rg")
    merge_neighbour_outputs(p, "ongoing")
    merge_neighbour_outputs(p, "ongoing_supervoxel_counts")
    merge_neighbour_outputs(p, "ongoing_semantic_labels")
    for meta in sys.argv[2:]:
        merge_neighbour_outputs(p, "ongoing_{}".format(meta))
    d = p["neighbours"]
    face_maps = {i : [d[k] for k in cu.generate_superface_keys(i) if k in d] for i in range(6)}
    merge_faces(p,face_maps)


param = cu.read_inputs(sys.argv[1])
ac_offset = param["ac_offset"]

print("mip level:", param["mip_level"])
merge_chunks(param)
for i in range(6):
    merge_cut_plane(param, i)

with open("chunk_offset.txt", "w") as f:
    f.write(str(ac_offset))
