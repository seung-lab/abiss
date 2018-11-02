import sys
from chunk_utils import read_inputs
from cut_chunk_common import load_data, cut_data, save_raw_data
import os
import numpy as np

param = read_inputs(sys.argv[1])
bbox = param["bbox"]
boundary_flags = [0,0,0,0,0,0]

start_coord = bbox[0:3]
end_coord = bbox[3:6]

seg = load_data(os.environ['WS_PATH'],mip=int(os.environ['WS_MIP']))
seg_cutout = cut_data(seg, start_coord, end_coord, boundary_flags)
save_raw_data("seg.raw", seg_cutout, seg.dtype)

del seg_cutout

aff = load_data(os.environ['AFF_PATH'],mip=int(os.environ['AFF_MIP']))
aff_cutout = cut_data(aff, start_coord, end_coord, boundary_flags)
idx = aff_cutout[:,:,:,0] < 0.01
save_raw_data("mask.raw", np.squeeze(idx).astype("uint8"), np.uint8)

#save_data("aff.h5", aff_cutout)
#save_data("seg.h5", seg_cutout)

