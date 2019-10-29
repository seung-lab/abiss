import json
import sys

def read_inputs(fn):
    with open(fn) as f:
        return json.load(f)

def chunk_tag(mip_level, indices):
    idx = [mip_level] + indices
    return "_".join([str(i) for i in idx])


param = read_inputs(sys.argv[1])
d = param["children"]
mip_c = param["mip_level"] - 1
for k in d:
    tag = chunk_tag(mip_c,d[k])
    print(tag)
