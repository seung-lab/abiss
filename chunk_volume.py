from cloudvolume import CloudVolume
from chunk_iterator import ChunkIterator
import sys
import json
import numpy as np

def get_chunk_offset(layer=None, x=None, y=None, z=None):
        """ (1) Extract Chunk ID from Node ID
            (2) Build Chunk ID from Layer, X, Y and Z components
        :param layer: int
        :param x: int
        :param y: int
        :param z: int
        :return: np.uint64
        """

        bits_per_dim = 8
        n_bits_for_layer_id = 8

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
        return np.uint64(layer << layer_offset | x << x_offset |
                         y << y_offset | z << z_offset)

def process_atomic_chunks(c, top_mip, ac_offset):
    x,y,z = c.coordinate()
    offset = get_chunk_offset(1, x, y, z)
    output = {
        "top_mip_level" : top_mip,
        "mip_level": c.mip_level(),
        "indices": c.coordinate(),
        "bbox": c.data_bbox(),
        "boundary_flags": c.boundary_flags(),
        "offset" : int(offset),
        "ac_offset" : ac_offset
    }
    fn = str(c.mip_level()) + "_" + "_".join([str(i) for i in c.coordinate()]) + ".json"
    with open(fn, 'w') as fp:
        json.dump(output, fp, indent=2)

def process_composite_chunks(c, top_mip, offset):
    d = c.possible_children()
    output = {
        "top_mip_level" : top_mip,
        "mip_level": c.mip_level(),
        "indices": c.coordinate(),
        "bbox": c.data_bbox(),
        "boundary_flags": c.boundary_flags(),
        "children": {k: v.coordinate() for k, v in d.items() if v.has_data()},
        "ac_offset" : offset
    }
    fn = str(c.mip_level()) + "_" + "_".join([str(i) for i in c.coordinate()]) + ".json"
    with open(fn, 'w') as fp:
        json.dump(output, fp, indent=2)

def process_composite_tasks(c, top_mip, f_task, f_deps):
    top_tag = str(top_mip)+"_0_0_0"
    tag = str(c.mip_level()) + "_" + "_".join([str(i) for i in c.coordinate()])
    d = c.children()
    bundle_size = 4;
    if c.mip_level() >= 4:
        f_task.write('generate_chunks["{}"]=composite_chunks_long_op(dag, ["{}"])\n'.format(tag,tag))
    else:
        f_task.write('generate_chunks["{}"]=composite_chunks_op(dag, ["{}"])\n'.format(tag,tag))
    if c.mip_level() > 1:
        for k in d:
            tag_c = str(c.mip_level()-1) + "_" + "_".join([str(i) for i in k.coordinate()])
            f_deps.write('generate_chunks["{}"].set_downstream(generate_chunks["{}"])\n'.format(tag_c, tag))
    else:
        tags_c = [str(c.mip_level()-1) + "_" + "_".join([str(i) for i in k.coordinate()]) for k in d]
        for i in range(0, len(tags_c), bundle_size):
            tags_b = '","'.join(tags_c[0+i:bundle_size+i])
            f_task.write('generate_chunks["{}_{}"]=atomic_chunks_op(dag, ["{}"])\n'.format(tag,i,tags_b))
            f_task.write('remap_chunks["{}_{}"]=remap_chunks_op(dag, ["{}"])\n'.format(tag,i,tags_b))
            f_deps.write('generate_chunks["{}_{}"].set_downstream(generate_chunks["{}"])\n'.format(tag,i,tag))
            f_deps.write('generate_chunks["{}"].set_downstream(remap_chunks["{}_{}"])\n'.format(top_tag,tag,i))
#cutout E
#data_bbox = [600, 16000, 10, 24000, 29000, 1500]
#PHASE I ROI
#data_bbox = [10240, 7680, 0, 65016, 43716, 1003]
#New pinky40 ROI
#data_bbox = [10240,4096,0,10240+57344,4096+40960,0+1024]
#Pinky 20K test
#data_bbox = [60000//2, 45000//2, 500, 100960//2, 65480//2, 1500]
#basil 10K
#data_bbox = [50000, 50000, 140, 50000+10240, 50000+10240, 890]
#data_bbox = [25000//2, 20000//2, 140, 45480//2, 40480//2, 940]
#data_bbox = [25000//2, 229520//2, 140, 45480//2, 250000//2, 940]
#New pinky40 MIP 1
#data_bbox = [54655//2+256, 39266//2+256, 1203+20, 112699//2-256, 74908//2-256, 2175-20]
#basil 4k
#data_bbox = [110076//2, 105862//2, 185, 114172//2, 109958//2, 285]
#data_bbox = [52535,87250,800,54583,89298,980]
#basil_6k
#data_bbox = [12822, 39273, 140, 15894, 42345, 620]
#data_bbox = [31500, 97600, 402, 34572, 100672, 882]
#data_bbox = [0,16000,0, 24000,30000,1500]
#data_bbox = [0,0,0,1250,1250,125]
#data_bbox = [10240,14336,10,19000,23000,1800]
#chunk_size = [512,512,512]
#basil
#data_bbox = [25000//2, 20000//2, 140, 165000//2, 250000//2, 940]

#basil-extended
#data_bbox = [25000//2, 20000//2, 8, 196990//2, 250000//2, 940]

#basil-extra1
#data_bbox = [173000//2, 20000//2, 140, 196990//2, 250000//2, 940]
#basil-extra2
#data_bbox = [173000//2, 20000//2, 8, 196990//2, 250000//2, 124]
#basil-extra3
#data_bbox = [25000//2, 20000//2, 8, 165000//2, 250000//2, 124]

#basil-defect1
#data_bbox = [165000//2+1, 20000//2, 8, 173000//2-1, 250000//2, 940]
#basil-defect2
#data_bbox = [25000//2, 20000//2, 125, 165000//2, 250000//2, 139]
#basil-defect3
#data_bbox = [173000//2, 20000//2, 125, 196990//2, 250000//2, 139]

#pinky_100
data_bbox = [35680//2+256, 30046//2+256, 1+20, 122630//2-256, 82140//2-256, 2177-20]
#data_bbox = [56404//2, 35020//2, 1+20, 120852//2, 80470//2, 2177-20]

#rizzo
#data_bbox = [15,15,2, 26624-15, 26624-15, 3093-2]

#eroded
#data_bbox = [90822//2, 63850//2, 1200,90822//2+1024, 63850//2+1024, 1200+128]

chunk_size = [512,512,128]

v = ChunkIterator(data_bbox, chunk_size)
#offset = chunk_size[0]*chunk_size[1]*chunk_size[2]
offset = 1<<32

top_mip = v.top_mip_level()

f_task = open("task.txt", "w")
f_deps = open("deps.txt", "w")

if (len(sys.argv) == 2):
    metadata_seg = CloudVolume.create_new_info(
            num_channels    = 1,
            layer_type      = 'segmentation',
            data_type       = 'uint64',
            encoding        = 'raw',
            resolution      = [8, 8, 40], # Pick scaling for your data!
            voxel_offset    = data_bbox[0:3],
            chunk_size      = [128,128,16], # This must divide evenly into image length or you won't cover the whole cube
            volume_size     = [data_bbox[i+3] - data_bbox[i] for i in range(3)]
            )

    vol_ws = CloudVolume(sys.argv[1], mip=0, info=metadata_seg)
    vol_ws.commit_info()

for c in v:
    if c.mip_level() == 0:
        process_atomic_chunks(c, top_mip, offset)
        #process_atomic_tasks(c, top_mip, f_task, f_deps)
    else:
        process_composite_chunks(c, top_mip, offset)
        process_composite_tasks(c, top_mip, f_task, f_deps)
