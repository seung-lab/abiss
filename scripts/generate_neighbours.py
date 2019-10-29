import sys
from chunk_utils import read_inputs, chunk_tag

param = read_inputs(sys.argv[1])
d = param["neighbours"]
mip = param["mip_level"]
for k in d:
    tag = chunk_tag(mip,d[k])
    print(tag)

