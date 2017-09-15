from chunk_iterator import ChunkIterator
import json

def process_atomic_chunks(c):
    output = {
        "mip_level": c.mip_level(),
        "indices": c.coordinate(),
        "bbox": c.data_bbox(),
        "boundary_flags": c.boundary_flags(),
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


v = ChunkIterator([0,0,0,2048,2048,256],[512,512,100])

for c in v:
    if c.mip_level() == 0:
        process_atomic_chunks(c)
    else:
        process_composite_chunks(c)
    c.print_chunk_info()
