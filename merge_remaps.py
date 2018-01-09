import sys
import struct
import os
import chunk_utils as cu

def read_meta(ancestors):
    remaps = 0
    for s in ancestors:
        fn = "meta_"+s+".data"
        data = open(fn, "rb").read()
        meta_data = struct.unpack("llllll", data)
        remaps += meta_data[5]

    return remaps

def merge_remaps(ancestor_tags, expected):
    fnList = ["remap_"+t+".data" for t in ancestor_tags]
    cu.merge_files("remap.data", fnList)
    filesize = os.path.getsize("remap.data")
    if filesize != expected:
        print("Something is wrong in remap, got {0} while expecting {1}".format(filesize, expected))

param = cu.read_inputs(sys.argv[1])
root = cu.read_inputs(sys.argv[2])

top_mip = root["mip_level"]
mip = param["mip_level"]
indices = param["indices"]
ancestors = []
ancestors.append(cu.chunk_tag(mip, indices))
while mip < top_mip:
    mip += 1
    indices = cu.parent(indices)
    ancestors.append(cu.chunk_tag(mip, indices))

ancestors = list(reversed(ancestors))

if param["mip_level"] == 0:
    bbox = param["bbox"]
    sizes = [bbox[i+3]-bbox[i] for i in range(3)]
    remaps = read_meta(ancestors)
    merge_remaps(ancestors, remaps*16)
    with open("param.txt","w") as f:
        f.write(" ".join([str(i) for i in sizes]))
        f.write("\n")
        f.write(str(remaps))
else:
    print("only atomic chunks need remapping")
