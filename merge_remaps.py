import sys
import struct
import os
import chunk_utils as cu

def read_meta(branches):
    remaps = 0
    for s in branches:
        fn = "meta_"+s+".data"
        data = open(fn, "rb").read()
        meta_data = struct.unpack("llllll", data)
        remaps += meta_data[5]

    return remaps

def merge_remaps(branches_tags, expected):
    fnList = ["remap_"+t+".data" for t in branches_tags]
    cu.merge_files("remap.data", fnList)
    filesize = os.path.getsize("remap.data")
    if filesize != expected*16:
        print("Something is wrong in remap, got {0} while expecting {1}".format(filesize, expected))

    return filesize

param = cu.read_inputs(sys.argv[1])

top_mip = param["top_mip_level"]
mip = param["mip_level"]
indices = param["indices"]

descedants = list(reversed(cu.generate_descedants(sys.argv[1])))

ancestors = list(cu.generate_ancestors(sys.argv[1]))

branches = descedants+[cu.chunk_tag(mip, indices)]+ancestors

print(branches)

bbox = param["bbox"]
sizes = [bbox[i+3]-bbox[i] for i in range(3)]
remaps = read_meta(branches)
actual_size = merge_remaps(branches, remaps)//16
print(actual_size)
with open("param.txt","w") as f:
    f.write(" ".join([str(i) for i in sizes]))
    f.write("\n")
    f.write(str(actual_size))
    f.write("\n")
    f.write("0")
