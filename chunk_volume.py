from chunk_iterator import ChunkIterator
import sys
import json
from joblib import Parallel, delayed
from chunk_utils import get_chunk_offset, chunk_voxels

def process_atomic_chunks(c, top_mip, ac_offset):
    x,y,z = c.coordinate()
    offset = get_chunk_offset(1, x, y, z)
    d = c.possible_neighbours()
    output = {
        "top_mip_level" : top_mip,
        "mip_level": c.mip_level(),
        "indices": c.coordinate(),
        "bbox": c.data_bbox(),
        "boundary_flags": c.boundary_flags(),
        "neighbours": {k: v.coordinate() for k, v in d.items() if v.has_data()},
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

def process_batch_chunk(sv, offset):
    for c in sv:
        if c.mip_level() == 0:
            process_atomic_chunks(c, top_mip, offset)
        else:
            process_composite_chunks(c, top_mip, offset)

root_tag = sys.argv[1]
with open(sys.argv[2]) as f:
    data = json.load(f)
    data_bbox = data["BBOX"]
    chunk_size = data["CHUNK_SIZE"]

a = [int(x) for x in root_tag.split("_")]
v = ChunkIterator(data_bbox, chunk_size, start_from=a)
#offset = chunk_size[0]*chunk_size[1]*chunk_size[2]

top_mip = v.top_mip_level()

batch_mip = 3

batch_chunks = []
if a[0] > batch_mip and top_mip > batch_mip:
    for c in v:
        if c.mip_level() < batch_mip:
            break
        elif c.mip_level() == batch_mip:
            batch_chunks.append(ChunkIterator(data_bbox, chunk_size, start_from = [batch_mip]+c.coordinate()))
        else:
            process_composite_chunks(c, top_mip, chunk_voxels)
else:
    batch_chunks=[v]

print("parallel starts: {}".format(len(batch_chunks)))
Parallel(n_jobs=-2)(delayed(process_batch_chunk)(sv, chunk_voxels) for sv in batch_chunks)
