from cut_chunk_common import cut_data
import chunk_utils as cu
import numpy
import os

def warp_z(z):
    return z
    #z_min = 256
    #z_range = 512
    #z_dist = z - z_min
    #if z_dist >= z_range:
    #    z_dist = z_dist % z_range

    #return z_min+z_dist

def fold_aff(a):
    #th = 0.99998
    #b = numpy.multiply(numpy.heaviside(th-a, 0),a)+numpy.multiply(numpy.heaviside(a-th, 1), th*2-a)
    #return b
    return a


def erode_affinitymap(data, params, threshold):
    from scipy import ndimage
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
    from scipy import ndimage
    #data[:,:,:,2] *= 0.99
    #idx = data[:] > 0.9
    #data[idx] *= 0.98
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


def adjust_affinitymap(aff, bbox, boundary_flags, padding_before, padding_after):
    global_param = cu.read_inputs(os.environ['PARAM_JSON'])

    if global_param.get("CLOSING_AFF", False):
        print("adjusting affinit map")
        erode_params = [[3,0.99], [3,0.99], [1,0.99]]
        start_coord = [bbox[i]-(1-boundary_flags[i])*(padding_before+erode_params[i][0]) for i in range(3)]
        end_coord = [bbox[i+3]+(1-boundary_flags[i+3])*(padding_after+erode_params[i][0]) for i in range(3)]
        data = erode_affinitymap(fold_aff(cut_data(aff, start_coord, end_coord, [0,0,0,0,0,0])), erode_params, 0.5)
        #bb = tuple(slice(start_coord[i], end_coord[i]) for i in range(3))
        #data = aff[bb+(slice(0,3),)]
        start_coord = [(1-boundary_flags[i])*erode_params[i][0] for i in range(3)]
        end_coord = [data.shape[i]-(1-boundary_flags[i+3])*erode_params[i][0] for i in range(3)]
        return cut_data(data, start_coord, end_coord, boundary_flags)
    elif "ADD_NOISE" in global_param:
        try:
            noise_level = float(global_param["ADD_NOISE"])
        except ValueError:
            print("pass through without adjusting affinit map")
            return fold_aff(cut_data(aff, start_coord, end_coord, boundary_flags))

        print(f"add noise from a normal distribution of standard deviation {noise_level}")
        start_coord = [bbox[i]-(1-boundary_flags[i])*padding_before for i in range(3)]
        end_coord = [bbox[i+3]+(1-boundary_flags[i+3])*padding_after for i in range(3)]
        data = cut_data(aff, start_coord, end_coord, boundary_flags)
        mask = data == 0
        data = numpy.add(data, numpy.random.normal(0, noise_level, data.shape))
        data[mask] = 0
        return data
    else:
        print("pass through without adjusting affinit map")
        start_coord = [bbox[i]-(1-boundary_flags[i])*padding_before for i in range(3)]
        end_coord = [bbox[i+3]+(1-boundary_flags[i+3])*padding_after for i in range(3)]
        return fold_aff(cut_data(aff, start_coord, end_coord, boundary_flags))


