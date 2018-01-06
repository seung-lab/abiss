import sys
from chunk_utils import read_inputs
from cut_chunk_common import load_data, cut_data, save_raw_data
import os

param = read_inputs(sys.argv[1])
bbox = param["bbox"]
boundary_flags = [0,0,0,0,0,0]

start_coord = bbox[0:3]
end_coord = bbox[3:6]

seg = load_data(os.environ['SEG_PATH'])
seg_cutout = cut_data(seg, start_coord, end_coord, boundary_flags)
save_raw_data("seg.raw", seg_cutout, seg.dtype)
#save_data("aff.h5", aff_cutout)
#save_data("seg.h5", seg_cutout)

