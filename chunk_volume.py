from chunk_iterator import ChunkIterator
import json

def process_atomic_chunks(c, idx):
    output = {
        "mip_level": c.mip_level(),
        "indices": c.coordinate(),
        "bbox": c.data_bbox(),
        "boundary_flags": c.boundary_flags(),
        "offset" : idx
    }
    fn = str(c.mip_level()) + "_" + "_".join([str(i) for i in c.coordinate()]) + ".json"
    with open(fn, 'w') as fp:
        json.dump(output, fp, indent=2)

def process_composite_chunks(c):
    d = c.possible_children()
    output = {
        "mip_level": c.mip_level(),
        "indices": c.coordinate(),
        "bbox": c.data_bbox(),
        "boundary_flags": c.boundary_flags(),
        "children": {k: v.coordinate() for k, v in d.iteritems() if v.has_data()},
    }
    fn = str(c.mip_level()) + "_" + "_".join([str(i) for i in c.coordinate()]) + ".json"
    with open(fn, 'w') as fp:
        json.dump(output, fp, indent=2)


data_bbox = [0,16000,0, 24000,30000,1500]
#data_bbox = [10240,14336,10,19000,23000,1800]
chunk_size = [1024,1024,512]
v = ChunkIterator(data_bbox,chunk_size)
offset = chunk_size[0]*chunk_size[1]*chunk_size[2]

index = 0
for c in v:
    if c.mip_level() == 0:
        process_atomic_chunks(c, index)
        index+=offset
    else:
        process_composite_chunks(c)
    c.print_chunk_info()
