from cloudvolume import CloudVolume
import numpy

def warp_z(z):
    return z
    #z_min = 256
    #z_range = 256
    #z_dist = z - z_min
    #if z_dist > z_range:
    #    z_dist = z_dist % z_range

    #return z_min+z_dist

def fold_aff(a):
    #b = numpy.multiply(numpy.heaviside(0.9-a, 0),a)+numpy.multiply(numpy.heaviside(a-0.9, 1), 1.8-a)
    #return b
    return a

def load_data(url, mip=0):
    print("cloud volume url: ", url)
    print("mip level: ", mip)
    return CloudVolume(url, fill_missing=True, mip=mip)
    #return CloudVolumeGSUtil(url, fill_missing=True)

def save_raw_data(fn,data, data_type):
    f = numpy.memmap(fn, dtype=data_type, mode='w+', order='F', shape=data.shape)
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

def cut_data(data, start_coord, end_coord, boundary_flags):
    bb = tuple(slice(start_coord[i], end_coord[i]) for i in range(3))
    if data.shape[3] == 1:
        return pad_data(data[bb], boundary_flags)
    elif data.shape[3] == 3:
        return pad_data(data[bb+(slice(0,3),)], boundary_flags)
    else:
        raise RuntimeError("encountered array of dimension " + str(len(data.shape)))


