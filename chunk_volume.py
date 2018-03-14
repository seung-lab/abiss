from chunk_iterator import ChunkIterator
import json

def process_atomic_chunks(c, top_mip, idx):
    output = {
        "top_mip_level" : top_mip,
        "mip_level": c.mip_level(),
        "indices": c.coordinate(),
        "bbox": c.data_bbox(),
        "boundary_flags": c.boundary_flags(),
        "offset" : idx
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
        "children": {k: v.coordinate() for k, v in d.iteritems() if v.has_data()},
        "ac_offset" : offset
    }
    fn = str(c.mip_level()) + "_" + "_".join([str(i) for i in c.coordinate()]) + ".json"
    with open(fn, 'w') as fp:
        json.dump(output, fp, indent=2)

def process_composite_tasks(c, top_mip, f_task, f_deps):
    top_tag = str(top_mip)+"_0_0_0"
    tag = str(c.mip_level()) + "_" + "_".join([str(i) for i in c.coordinate()])
    d = c.children()
    bundle_size = 8;
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

#data_bbox = [0,16000,0, 24000,30000,1500]
data_bbox = [0,0,0,2048,2048,256]
#data_bbox = [10240,14336,10,19000,23000,1800]
chunk_size = [1024,1024,512]
v = ChunkIterator(data_bbox,chunk_size)
offset = chunk_size[0]*chunk_size[1]*chunk_size[2]

top_mip = v.top_mip_level()

f_task = open("task.txt", "w")
f_deps = open("deps.txt", "w")
print([data_bbox[i+3] - data_bbox[i] for i in range(3)])

index = 0
for c in v:
    if c.mip_level() == 0:
        process_atomic_chunks(c, top_mip, index)
        #process_atomic_tasks(c, top_mip, f_task, f_deps)
        index+=offset
    else:
        process_composite_chunks(c, top_mip, offset)
        process_composite_tasks(c, top_mip, f_task, f_deps)
