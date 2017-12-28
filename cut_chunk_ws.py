from cloudvolume import CloudVolume
import sys
import chunk_utils as cu
import numpy
import os

def load_data(fn):
    return numpy.memmap("aff64.raw", dtype='float64', mode='r', order='F', shape=(2048,2048,256,3))
    #return CloudVolume(os.environ['AFF_PATH'])
    #return CloudVolume('gs://neuroglancer/drosophila_v0/affinitymap-aligned_z32', fill_missing=True)

def save_raw_data(fn,data):
    f = numpy.memmap(fn, dtype='float64', mode='w+', order='F', shape=data.shape)
    f[:] = data[:]
    del f

def pad_boundary(boundary_flags):
    pad = [[0,0],[0,0],[0,0]]
    for i in range(3):
        if boundary_flags[i] == 1:
            pad[i][0] = 1
        if boundary_flags[i+3] == 1:
            pad[i][1] = 1
    return list(pad)

def pad_data(data, boundary_flags):
    pad = pad_boundary(boundary_flags)
    if len(data.shape) == 3:
        return numpy.lib.pad(data, pad, 'constant', constant_values=0)
    elif len(data.shape) == 4:
        return numpy.lib.pad(data, pad+[[0,0]], 'constant', constant_values=0)
    else:
        raise RuntimeError("encountered array of dimension " + str(len(data.shape)))

def cut_data(data, bbox, boundary_flags):
    #offset = bbox[0:3]
    start_coord= [bbox[i]-1+boundary_flags[i] for i in range(3)]
    end_coord = [bbox[i+3]+1-boundary_flags[i+3] for i in range(3)]
    #size = [bbox[i+3] - bbox[i] + (1 - boundary_flags[i+3])*2 for i in range(3)]
    bb = tuple(slice(start_coord[i], end_coord[i]) for i in range(3))
    print(bb)
    #return data[(slice(0,3),)+bb]
    if data.shape[3] == 1:
        return pad_data(data[bb], boundary_flags)
    elif data.shape[3] == 3:
        return pad_data(data[bb+(slice(0,3),)], boundary_flags)
    else:
        raise RuntimeError("encountered array of dimension " + str(len(data.shape)))

def write_metadata(fn, size, boundaries, offset):
    with open(fn, "w") as f:
        f.write(" ".join([str(x) for x in size]))
        f.write("\n")
        f.write(" ".join([str(x) for x in boundaries]))
        f.write("\n")
        f.write(str(offset))

param = cu.read_inputs(sys.argv[2])
bbox = param["bbox"]
dims = [bbox[i+3] - bbox[i] for i in range(3)]
print(dims)
boundary_flags = param["boundary_flags"]
offset = param["offset"]

aff = load_data(sys.argv[1])

aff_cutout = cut_data(aff, bbox, boundary_flags)

#if bbox[2] == 0:
#    print("mask_affinity:",bbox)
#    aff_cutout[:,:,1:11,:] = 0

save_raw_data("aff.raw", aff_cutout)

write_metadata("param.txt", aff_cutout.shape[0:3], boundary_flags, offset)
