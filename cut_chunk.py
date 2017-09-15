import h5py
import sys
import json
import numpy

def load_data(fn):
    f = h5py.File(fn,'r')
    return f['main']

def save_data(fn, data):
    f = h5py.File(fn,"w")
    ds = f.create_dataset('main', data.shape)
    ds[:] = data
    f.close()

def save_raw_data(fn,data):
    f = open(fn,"w")
    data.tofile(f)
    f.close()

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
    return list(reversed(pad))

def pad_data(data, boundary_flags):
    pad = pad_boundary(boundary_flags)
    if len(data.shape) == 3:
        return numpy.lib.pad(data, pad, 'constant', constant_values=0)
    elif len(data.shape) == 4:
        return numpy.lib.pad(data, [[0,0]]+pad, 'constant', constant_values=0)
    else:
        raise RuntimeError("encountered array of dimension " + str(len(data.shape)))

def cut_data(data, bbox, boundary_flags):
    offset = bbox[0:3]
    size = [bbox[i+3] - bbox[i] + 1 - boundary_flags[i+3] for i in range(3)]
    bb = tuple(slice(offset[i], offset[i]+size[i]) for i in reversed(range(3)))
    if len(data.shape) == 3:
        return pad_data(data[bb], boundary_flags)
    elif len(data.shape) == 4:
        return pad_data(data[(slice(0,3),)+bb], boundary_flags)
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

aff = load_data(sys.argv[1])
seg = load_data(sys.argv[2])

aff_cutout = cut_data(aff, bbox, boundary_flags)
seg_cutout = cut_data(seg, bbox, boundary_flags)

save_raw_data("aff.raw", aff_cutout)
save_raw_data("seg.raw", seg_cutout)
#save_data("aff.h5", aff_cutout)
#save_data("seg.h5", seg_cutout)

write_metadata("param.txt", offset(bbox), reversed(seg_cutout.shape))
