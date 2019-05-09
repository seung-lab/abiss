import sys
import chunk_utils as cu
from cut_chunk_common import load_data, cut_data, save_raw_data, warp_z, fold_aff
import numpy
from scipy import ndimage
import os
from cloudvolume import CloudVolume

def write_metadata(fn, size, boundaries, offset):
    with open(fn, "w") as f:
        f.write(" ".join([str(x) for x in size]))
        f.write("\n")
        f.write(" ".join([str(x) for x in boundaries]))
        f.write("\n")
        f.write(str(offset))

def adjust_affinitymap(data, params, threshold):
    data[:,:,:,2] *= 0.99
    idx = data[:] > 0.9
    data[idx] = 0.9
    for i in range(3):
        mask = numpy.ones_like(data[:,:,:,i])
        idx = data[:,:,:,i] < threshold
        mask[idx] = 0
        step = -2
        if (i == 2):
            step = -2
        for r in (range(params[i][0]*2+1, 2, step)):
            sz = [0,0,0]
            sz[i] = r
            print(sz)
            lr_mask = ndimage.grey_erosion(mask, size=sz)
            lr_idx = lr_mask < threshold
            data[lr_idx,:] *= params[i][1]

    return data


param = cu.read_inputs(sys.argv[1])
bbox = param["bbox"]
aff_bbox = bbox[:]
aff_bbox[2] = warp_z(bbox[2])
aff_bbox[5] = aff_bbox[2] + (bbox[5] - bbox[2])
print(bbox)
print(aff_bbox)
boundary_flags = param["boundary_flags"]
offset = param["offset"]

aff = load_data(os.environ['AFF_PATH'],mip=int(os.environ['AFF_MIP']))
start_coord = [aff_bbox[i]-1+boundary_flags[i] for i in range(3)]
end_coord = [aff_bbox[i+3]+1-boundary_flags[i+3] for i in range(3)]
aff_cutout = fold_aff(cut_data(aff, start_coord, end_coord, boundary_flags))
#erode_params = [[20,0.995],[20,0.995],[2,0.99]]
#start_coord = [bbox[i]-(1-boundary_flags[i])*(1+erode_params[i][0]) for i in range(3)]
#end_coord = [bbox[i+3]+(1-boundary_flags[i+3])*(1+erode_params[i][0]) for i in range(3)]
#bb = tuple(slice(start_coord[i], end_coord[i]) for i in range(3))
#data = adjust_affinitymap(aff[bb+(slice(0,3),)], erode_params, 0.8)
##data = aff[bb+(slice(0,3),)]
#shape = data.shape
#start_coord = [(1-boundary_flags[i])*erode_params[i][0] for i in range(3)]
#end_coord = [shape[i]-(1-boundary_flags[i+3])*erode_params[i][0] for i in range(3)]
#aff_cutout = cut_data(data, start_coord, end_coord, boundary_flags)

save_raw_data("aff.raw", aff_cutout, aff.dtype)

write_metadata("param.txt", aff_cutout.shape[0:3], boundary_flags, offset)

fn = "remap/done_{}_{}.data".format(cu.chunk_tag(param['mip_level'],param['indices']), offset)
f = open(fn,"w")
f.close()
#upload data
#vol = CloudVolume(os.environ['ERODED_AFF_PATH'], mip=0, cdn_cache=False)
#vol[bbox[0]:bbox[3], bbox[1]:bbox[4], bbox[2]:bbox[5], :] = aff_cutout[1:-1, 1:-1, 1:-1, :]

