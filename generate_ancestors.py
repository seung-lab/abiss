import sys
import chunk_utils as cu

p = cu.read_inputs(sys.argv[1])
r = cu.read_inputs(sys.argv[2])
top_mip = r["mip_level"]
mip = p["mip_level"]
indices = p["indices"]
print(cu.chunk_tag(mip, indices))
while mip < top_mip:
    mip += 1
    indices = cu.parent(indices)
    print(cu.chunk_tag(mip, indices))
