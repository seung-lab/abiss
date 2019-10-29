import sys
import chunk_utils as cu

def merge_size(ancestor_tags, offset):
    fnList = []
    for a in ancestor_tags:
        fnList.append("size_{}_{}.data".format(a,offset))

    print(fnList)
    cu.merge_files("size.data", fnList)

param = cu.read_inputs(sys.argv[1])
#print(ancestors)

if param["mip_level"] == 0:
    ancestors = cu.generate_ancestors(sys.argv[1])
    offset = param["offset"]
    ancestors = list(ancestors)
    merge_size(ancestors, offset)
else:
    print("only atomic chunks need remapping")
