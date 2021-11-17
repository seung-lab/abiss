import sys
import os
import chunk_utils as cu

def merge_remaps(prefixes, ancestor_tags, offset):
    content = b''
    for a in ancestor_tags:
        for p in prefixes:
            payload = cu.download_slice(p, a, offset)
            if payload:
                content += payload

    with open('remap.data','wb') as out:
        out.write(content)

    return len(content)

param = cu.read_inputs(sys.argv[1])
global_param = cu.read_inputs(os.environ['PARAM_JSON'])

ancestors = cu.generate_ancestors(sys.argv[1])
ancestors = list(ancestors)
if os.environ["STAGE"] == "ws":
    prefixes = ['remap/done_pre', 'remap/done_post']
    ancestors = ancestors[1:]
elif os.environ["STAGE"] == "agg":
    prefixes = ['remap/done']

offset = param["offset"]

chunked_agg_output = False
if len(sys.argv) > 2:
    chunked_agg_output = bool(sys.argv[2])

#print(ancestors)

if param["mip_level"] == 0:
    bbox = param["bbox"]
    sizes = [bbox[i+3]-bbox[i] for i in range(3)]
    actual_size = merge_remaps(prefixes, ancestors, offset)//16
    print(actual_size)
    with open("param.txt","w") as f:
        f.write(" ".join([str(i) for i in sizes]))
        f.write("\n")
        f.write(str(actual_size))
        f.write("\n")
        if os.environ["STAGE"] == "ws" or chunked_agg_output:
            f.write("1")
        else:
            f.write("0")
else:
    print("only atomic chunks need remapping")
