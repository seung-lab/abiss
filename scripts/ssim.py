import numpy as np
import chunk_utils as cu
import sys
import os
from skimage import segmentation, morphology,measure
from cloudvolume import CloudVolume

param = cu.read_inputs(sys.argv[1])
bbox = param["bbox"]
sizes = tuple([bbox[i+3]-bbox[i] for i in range(3)])
fn = "seg_"+cu.chunk_tag(param["mip_level"],param["indices"])+".data"
fp_seg = np.memmap(fn, dtype='uint64', mode='r', shape=sizes, order='F')
fp_mask = np.memmap("mask.raw", dtype='uint8', mode='r', shape=sizes, order='F')

mask = np.array(fp_mask)
ssim = np.zeros(sizes, dtype='float64')
for i in range(128):
    bimg = segmentation.find_boundaries(fp_seg[:,:,i], mode='thick')
    boundary = morphology.binary_dilation(np.squeeze(bimg),morphology.square(2)).astype("uint8")[:]
    ssimval, ssimimg = measure.compare_ssim(np.squeeze(mask[:,:,i]), np.squeeze(boundary), win_size=11, full=True)
    #print(ssimval)
    ssim[:,:,i] = ssimimg.reshape(bimg.shape)[:]

qssim = 255*20*(1.0-ssim)
idx = qssim[:] > 255
qssim[idx] = 255


vol = CloudVolume(os.environ['SSIM_PATH'],mip=int(os.environ['SSIM_MIP']),cdn_cache=False)
vol[bbox[0]:bbox[3], bbox[1]:bbox[4], bbox[2]:bbox[5]] = qssim.astype("uint8")[:]
