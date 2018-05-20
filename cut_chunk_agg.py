import sys
from chunk_utils import read_inputs
from cut_chunk_common import load_data, cut_data, save_raw_data
import os

def chunk_origin(bbox):
    offset = bbox[0:3]
    for i in range(3):
        if boundary_flags[i] == 1:
            offset[i] -= 1
    return offset

def write_metadata(fn, offset, size):
    with open(fn, "w") as f:
        f.write(" ".join([str(x) for x in offset]))
        f.write("\n")
        f.write(" ".join([str(x) for x in size]))

param = read_inputs(sys.argv[1])
bbox = param["bbox"]
boundary_flags = param["boundary_flags"]

aff = load_data(os.environ['AFF_PATH'],mip=0)
#aff = numpy.memmap("../aff64.raw", dtype='float64', mode='r', order='F', shape=(2048,2048,256,3))
start_coord = bbox[0:3]
end_coord = [bbox[i+3]+1-boundary_flags[i+3] for i in range(3)]

aff_cutout = cut_data(aff, start_coord, end_coord, boundary_flags)
save_raw_data("aff.raw", aff_cutout, aff.dtype)
del aff_cutout

seg = load_data(os.environ['WS_PATH'],mip=0)
seg_cutout = cut_data(seg, start_coord, end_coord, boundary_flags)
save_raw_data("seg.raw", seg_cutout, seg.dtype)
#save_data("aff.h5", aff_cutout)
#save_data("seg.h5", seg_cutout)

write_metadata("param.txt", chunk_origin(bbox), seg_cutout.shape[0:3])
