import sys
import os
import numpy as np
import chunk_utils as cu
from cloudvolume import CloudVolume

param = cu.read_inputs(sys.argv[1])
global_param = cu.read_inputs(os.environ['PARAM_JSON'])

fn = "seg_"+cu.chunk_tag(param["mip_level"],param["indices"])+".data"
print(fn)
bbox = param["bbox"]
sizes = tuple([bbox[i+3]-bbox[i] for i in range(3)])
fp = np.memmap(fn, dtype=np.uint64, mode='r', shape=sizes, order='F')
data = np.array(fp[:])
if global_param.get("REMOVE_SMALL_SEGMENTS", False) and os.environ["STAGE"] == "agg":
    fn_size = "size_map.data"
    fp_size = np.memmap(fn_size, dtype=np.uint8, mode='r', shape=sizes, order='F')
    data_size = np.array(fp_size[:])
    data[data_size > 0] = 0
vol = CloudVolume(sys.argv[2], mip=global_param['AFF_RESOLUTION'], cdn_cache=False)
vol[bbox[0]:bbox[3], bbox[1]:bbox[4], bbox[2]:bbox[5]] = data[:]
