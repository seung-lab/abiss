import sys
from chunk_utils import read_inputs
from cut_chunk_common import load_data, cut_data, save_raw_data, warp_z, fold_aff, adjust_affinitymap
import os

def chunk_origin(bbox):
    offset = bbox[0:3]
    for i in range(3):
        #if boundary_flags[i] == 1:
            offset[i] -= 1
    return offset

def write_metadata(fn, offset, size, ac_offset, flags):
    with open(fn, "w") as f:
        f.write(" ".join([str(x) for x in offset]))
        f.write("\n")
        f.write(" ".join([str(x) for x in size]))
        f.write("\n")
        f.write(str(ac_offset))
        f.write("\n")
        f.write(" ".join([str(x) for x in flags]))

param = read_inputs(sys.argv[1])
global_param = read_inputs(os.environ['PARAM_JSON'])
bbox = param["bbox"]
print(bbox)
offset = param["offset"]
boundary_flags = param["boundary_flags"]

#start_coord = bbox[0:3]
start_coord = [bbox[i]-1+boundary_flags[i] for i in range(3)]
end_coord = [bbox[i+3]+1-boundary_flags[i+3] for i in range(3)]

seg = load_data(os.environ['SEG_PATH'], mip=global_param['AFF_RESOLUTION'], fill_missing=True)
seg_cutout = cut_data(seg, start_coord, end_coord, boundary_flags)
save_raw_data("seg.raw", seg_cutout, seg.dtype)


write_metadata("param.txt", chunk_origin(bbox), seg_cutout.shape[0:3], offset, boundary_flags)
