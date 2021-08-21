import sys
import os
import chunk_utils as cu

def merge_remaps(ancestor_tags, offset):
    content = b''
    for a in ancestor_tags:
        payload = cu.download_slice('remap/done', a, offset)
        if payload:
            content += payload

    with open('remap.data','wb') as out:
        out.write(content)

    return len(content)

param = cu.read_inputs(sys.argv[1])
global_param = cu.read_inputs(os.environ['PARAM_JSON'])

ancestors = cu.generate_ancestors(sys.argv[1])
s = int(sys.argv[2])

offset = param["offset"]

ancestors = list(ancestors)
#print(ancestors)

if param["mip_level"] == 0:
    bbox = param["bbox"]
    sizes = [bbox[i+3]-bbox[i] for i in range(3)]
    actual_size = merge_remaps(ancestors[s:], offset)//16
    print(actual_size)
    with open("param.txt","w") as f:
        f.write(" ".join([str(i) for i in sizes]))
        f.write("\n")
        f.write(str(actual_size))
        f.write("\n")
        if os.environ["STAGE"] == "ws" and global_param.get("CHUNK_OUTPUT", True):
            f.write("1")
        elif os.environ["STAGE"] == "agg" and global_param.get("CHUNK_OUTPUT", False):
            f.write("1")
        else:
            f.write("0")
else:
    print("only atomic chunks need remapping")
