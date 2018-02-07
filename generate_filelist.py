import sys
import chunk_utils as cu

p = cu.read_inputs(sys.argv[1])
ancestors = cu.generate_ancestors(sys.argv[1])
offset = p["offset"]
for a in ancestors:
    print("done_{}_{}".format(a,offset))
