from cloudvolume import CloudVolume
import sys
import json
import numpy

def load_data(url):
    return CloudVolume(url)

def save_raw_data(fn,data, data_type):
    f = numpy.memmap(fn, dtype=data_type, mode='w+', order='F', shape=data.shape)
    f[:] = data[:]
    del f

def read_inputs(fn):
    with open(fn) as f:
        return json.load(f)

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
    offset = bbox[0:3]
    size = [bbox[i+3] - bbox[i] + 1 - boundary_flags[i+3] for i in range(3)]
    bb = tuple(slice(offset[i], offset[i]+size[i]) for i in range(3))
    if data.shape[3] == 1:
        return pad_data(data[bb], boundary_flags)
    elif data.shape[3] == 3:
        return pad_data(data[bb+(slice(0,3),)], boundary_flags)
    else:
        raise RuntimeError("encountered array of dimension " + str(len(data.shape)))

def offset(bbox):
    offset = bbox[0:3]
    for i in range(3):
        if boundary_flags[i] == 1:
            offset[i] = -1
    return offset

def write_metadata(fn, offset, size):
    with open(fn, "w") as f:
        f.write(" ".join([str(x) for x in offset]))
        f.write("\n")
        f.write(" ".join([str(x) for x in size]))

param = read_inputs(sys.argv[3])
bbox = param["bbox"]
boundary_flags = param["boundary_flags"]

aff = load_data('gs://neuroglancer/drosophila_v0/affinitymap-aligned')
aff_cutout = cut_data(aff, bbox, boundary_flags)
save_raw_data("aff.raw", aff_cutout, "float32")
del aff_cutout

seg = load_data('gs://neuroglancer/ranl/watershed_13')
seg_cutout = cut_data(seg, bbox, boundary_flags)
save_raw_data("seg.raw", seg_cutout, "uint64")
#save_data("aff.h5", aff_cutout)
#save_data("seg.h5", seg_cutout)

write_metadata("param.txt", offset(bbox), seg_cutout.shape[0:3])
