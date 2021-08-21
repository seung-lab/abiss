import sys
import chunk_utils as cu

def merge_size(ancestor_tags, offset):
    content = b''
    for a in ancestor_tags:
        payload = cu.download_slice('remap/size', a, offset)
        if payload:
            content += payload

    with open('size.data','wb') as out:
        out.write(content)

param = cu.read_inputs(sys.argv[1])
#print(ancestors)

if param["mip_level"] == 0:
    ancestors = cu.generate_ancestors(sys.argv[1])
    offset = param["offset"]
    ancestors = list(ancestors)
    merge_size(ancestors, offset)
else:
    print("only atomic chunks need remapping")
