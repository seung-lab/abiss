import sys
import chunk_utils as cu

p = cu.read_inputs(sys.argv[1])
mip = p["mip_level"]
indices = p["indices"]
print(cu.chunk_tag(mip, indices))

d = cu.generate_ancestors(sys.argv[1]) + cu.generate_descedants(sys.argv[1])
for c in d:
    print(c)
