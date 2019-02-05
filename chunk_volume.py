from cloudvolume import CloudVolume
from chunk_iterator import ChunkIterator
import sys
import json
from chunk_utils import get_chunk_offset
from dataset import chunk_size

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


with open(sys.argv[1]) as f:
    data = json.load(f)
    data_bbox = data["BBOX"]

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
