import sys
import os
import numpy as np
import chunk_utils as cu
from cloudvolume import CloudVolume

param = cu.read_inputs(sys.argv[1])

if param["mip_level"] == 0:
    fn = "seg_"+cu.chunk_tag(param["mip_level"],param["indices"])+".data"
    print(fn)
    bbox = param["bbox"]
    sizes = tuple([bbox[i+3]-bbox[i] for i in range(3)])
    fp = np.memmap(fn, dtype=np.uint64, mode='r', shape=sizes, order='F')
    vol = CloudVolume(os.environ['SEG_PATH'])
    vol[bbox[0]:bbox[3], bbox[1]:bbox[4], bbox[2]:bbox[5]] = fp[:]
