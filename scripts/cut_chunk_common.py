from cloudvolume import CloudVolume
import chunk_utils as cu
import numpy
import os
from scipy import ndimage

def warp_z(z):
    return z
    #z_min = 256
    #z_range = 256
    #z_dist = z - z_min
    #if z_dist >= z_range:
    #    z_dist = z_dist % z_range

    #return z_min+z_dist

def fold_aff(a):
    #b = numpy.multiply(numpy.heaviside(0.9-a, 0),a)+numpy.multiply(numpy.heaviside(a-0.9, 1), 1.8-a)
    return a

def adjust_affinitymap(aff, bbox, boundary_flags, padding_before, padding_after):
    global_param = cu.read_inputs(os.environ['PARAM_JSON'])
    if not global_param.get("CLOSING_AFF", False):
        start_coord = [bbox[i]-(1-boundary_flags[i])*padding_before for i in range(3)]
        end_coord = [bbox[i+3]+(1-boundary_flags[i+3])*padding_after for i in range(3)]
        return fold_aff(cut_data(aff, start_coord, end_coord, boundary_flags))

    erode_params = [[10,0.99],[10,0.99],[2,0.99]]
    start_coord = [bbox[i]-(1-boundary_flags[i])*(padding_before+erode_params[i][0]) for i in range(3)]
    end_coord = [bbox[i+3]+(1-boundary_flags[i+3])*(padding_after+erode_params[i][0]) for i in range(3)]
    data = close_affinitymap(cut_data(aff, start_coord, end_coord, [0,0,0,0,0,0]), erode_params, 0.5)
    #bb = tuple(slice(start_coord[i], end_coord[i]) for i in range(3))
    #data = aff[bb+(slice(0,3),)]
    start_coord = [(1-boundary_flags[i])*erode_params[i][0] for i in range(3)]
    end_coord = [data.shape[i]-(1-boundary_flags[i+3])*erode_params[i][0] for i in range(3)]
    return cut_data(data, start_coord, end_coord, boundary_flags)


def erode_affinitymap(data, params, threshold):
    #data[:,:,:,2] *= 0.99
    #idx = data[:] > 0.9
    #data[idx] = 0.9
    for i in range(3):
        mask = numpy.ones_like(data[:,:,:,i])
        idx = (data[:,:,:,i] < threshold) & (data[:,:,:,i] > 0)
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


def close_affinitymap(data, params, threshold):
    #data[:,:,:,2] *= 0.99
    idx = data[:] > 0.9
    data[idx] *= 0.98
    for i in range(3):
        mask = numpy.logical_and(data[:,:,:,i] < threshold, data[:,:,:,i] > 0)
        step = -2
        if (i == 2):
            step = -2
        for r in (range(params[i][0]*2+1, 2, step)):
            if i == 2:
                s_z = numpy.zeros((r,r,r), dtype=int)
                s_z[r//2,r//2,:] = 1
                lr_mask = numpy.logical_and(ndimage.binary_closing(mask, structure=s_z), numpy.logical_not(mask))
                data[lr_mask,i] *= params[i][1]
            else:
                s_xy = numpy.zeros((r,r,r), dtype=int)
                s_xy[:,:,r//2] = ndimage.iterate_structure(ndimage.generate_binary_structure(2, 1),r//2).astype(int)
                s_x = numpy.zeros((r,r,r), dtype=int)
                s_x[r//2,:,r//2] = 1
                s_y = numpy.zeros((r,r,r), dtype=int)
                s_y[:,r//2,r//2] = 1
                s_d = numpy.zeros((r,r,r), dtype=int)
                s_d[:,:,r//2] = numpy.eye(r, dtype=int)
                s_ad = numpy.zeros((r,r,r), dtype=int)
                s_ad[:,:,r//2] = numpy.flipud(numpy.eye(r, dtype=int))
                lr_mask_small = numpy.logical_or(numpy.logical_and(ndimage.binary_closing(mask, structure=s_x), ndimage.binary_closing(mask, structure=s_y)), numpy.logical_and(ndimage.binary_closing(mask, structure=s_d), ndimage.binary_closing(mask, structure=s_ad)))
                lr_mask = ndimage.binary_closing(mask, structure=s_xy)
                if r > 3:
                    lr_mask = numpy.logical_and(lr_mask, numpy.logical_not(lr_mask_small))
                data[lr_mask,i] *= params[i][1]

    return data


def load_data(url, **kwargs):
    print("cloud volume url: ", url)

    return CloudVolume(url, **kwargs)
    #return CloudVolumeGSUtil(url, fill_missing=True)

def load_gt_data(url, mip=0):
    print("cloud volume url: ", url)
    print("mip level: ", mip)

    return CloudVolume(url, fill_missing=True, bounded=False, mip=mip)

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
    elif data.shape[3] == 4: #0-2 affinity, 3 myelin
        global_param = cu.read_inputs(os.environ['PARAM_JSON'])
        th = global_param.get('MYELIN_THRESHOLD', 0.3)
        print("threshold myelin mask at {}".format(th))
        cutout = data[bb+(slice(0,4),)]
        affinity = cutout[:,:,:,0:3]
        myelin = cutout[:,:,:,3]
        mask = myelin > th
        for i in range(3):
            tmp = affinity[:,:,:,i]
            tmp[mask] = 0
            #affinity[:,:,:,i] = numpy.multiply(affinity[:,:,:,i]*(1-myelin))
        return pad_data(affinity, boundary_flags)

    else:
        raise RuntimeError("encountered array of dimension " + str(len(data.shape)))


