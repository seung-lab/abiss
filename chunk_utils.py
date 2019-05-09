import json
import os
import shutil

bits_per_dim = 10
n_bits_for_layer_id = 8

chunk_voxels = 1 << (64-n_bits_for_layer_id-bits_per_dim*3)

def get_chunk_offset(layer=None, x=None, y=None, z=None):
        """ (1) Extract Chunk ID from Node ID
            (2) Build Chunk ID from Layer, X, Y and Z components
        :param layer: int
        :param x: int
        :param y: int
        :param z: int
        :return: np.uint64
        """

        if not(x < 2 ** bits_per_dim and
               y < 2 ** bits_per_dim and
               z < 2 ** bits_per_dim):
            raise Exception("Chunk coordinate is out of range for"
                            "this graph on layer %d with %d bits/dim."
                            "[%d, %d, %d]; max = %d."
                            % (layer, bits_per_dim, x, y, z,
                               2 ** bits_per_dim))

        layer_offset = 64 - n_bits_for_layer_id
        x_offset = layer_offset - bits_per_dim
        y_offset = x_offset - bits_per_dim
        z_offset = y_offset - bits_per_dim
        return (layer << layer_offset | x << x_offset |
                         y << y_offset | z << z_offset)


def read_inputs(fn):
    with open(fn) as f:
        return json.load(f)

def chunk_tag(mip_level, indices):
    idx = [mip_level] + indices
    return "_".join([str(i) for i in idx])

def parent(indices):
    return [i//2 for i in indices]

def generate_subface_keys(idx):
    pos = idx % 3
    offset = idx // 3
    faces = [[i,j] for i in range(2) for j in range(2)]
    list(map(lambda l: l.insert(pos, offset), faces))
    return ["_".join([str(i) for i in l]) for l in faces]

def merge_files(target, fnList):
    if len(fnList) == 1:
        os.rename(fnList[0],target)
        return

    with open(target,"wb") as outfile:
        for fn in fnList:
            try:
                shutil.copyfileobj(open(fn, 'rb'), outfile)
            except IOError:
                print(fn, " does not exist")
                pass

    for fn in fnList:
        try:
            os.remove(fn)
        except IOError:
            print(fn, " does not exist")
            pass

def lift_intermediate_outputs(p, prefix):
    d = p["children"]
    mip_c = p["mip_level"]-1
    inputs = [prefix+"_"+chunk_tag(mip_c, d[k])+".data" for k in d]
    output = prefix+"_"+chunk_tag(p["mip_level"], p["indices"])+".data"
    merge_files(output, inputs)

def merge_intermediate_outputs(p, prefix):
    d = p["children"]
    mip_c = p["mip_level"]-1
    inputs = [prefix+"_"+chunk_tag(mip_c, d[k])+".data" for k in d]
    output = prefix+".data"
    merge_files(output, inputs)

def generate_ancestors(f, target=None, ceiling=None):
    p = read_inputs(f)
    top_mip = p["top_mip_level"]
    if ceiling is not None:
        top_mip = ceiling
    mip = p["mip_level"]
    indices = p["indices"]
    ancestor = [chunk_tag(mip, indices)]
    while mip < top_mip:
        mip += 1
        indices = parent(indices)
        if target is None or mip == target:
            ancestor.append(chunk_tag(mip, indices))

    return ancestor

def generate_siblings(f):
    param = read_inputs(f)
    indices = param["indices"]
    mip = param["mip_level"]
    boundary_flags = param["boundary_flags"]

    volume = [[0,0,0]]
    faces = []
    edges = []
    vertices = []
    for i, f in enumerate(boundary_flags[3:]):
        if f == 0:
            new_faces = [volume[0][:]]
            new_faces[0][i] += 1
            new_edges = []
            for a in faces:
                c = a[:]
                c[i] += 1
                new_edges.append(c)
            new_vertices = []
            for b in edges:
                c = b[:]
                c[i] += 1
                new_vertices.append(c)
            faces+=new_faces
            edges+=new_edges
            vertices+=new_vertices

    return mip, indices, volume, faces, edges, vertices

def generate_descedants(f, target=None):
    path = os.path.dirname(f)
    p = read_inputs(f)
    mip = p["mip_level"]
    if mip == 0:
        return []
    else:
        mip_c = mip - 1
        d = p["children"]
        descedants = []
        for k in d:
            tag = chunk_tag(mip_c, d[k])
            if target is None or mip_c == target:
                descedants.append(tag)

        for k in d:
            tag = chunk_tag(mip_c, d[k])
            f_c = os.path.join(path, tag+".json")
            descedants += generate_descedants(f_c, target)

        return descedants

