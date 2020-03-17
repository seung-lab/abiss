import sys
import numpy as np
import chunk_utils as cu
import os
from cloudvolume import CloudVolume

param = cu.read_inputs(sys.argv[1])
global_param = cu.read_inputs(os.environ['PARAM_JSON'])

fn = "size_map.data"
print(fn)
bbox = param["bbox"]
sizes = tuple([bbox[i+3]-bbox[i] for i in range(3)])
fp = np.memmap(fn, dtype=np.uint8, mode='r', shape=sizes, order='F')
vol = CloudVolume(sys.argv[2], mip=global_param['AFF_RESOLUTION'], cdn_cache=False)
vol[bbox[0]:bbox[3], bbox[1]:bbox[4], bbox[2]:bbox[5]] = fp[:]
