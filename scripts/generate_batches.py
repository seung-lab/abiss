from chunkiterator import ChunkIterator
import sys
import json

root_tag = sys.argv[1]
with open(sys.argv[2]) as f:
    data = json.load(f)
    data_bbox = data["BBOX"]
    chunk_size = data["CHUNK_SIZE"]

a = [int(x) for x in root_tag.split("_")]
print(a)
v = ChunkIterator(data_bbox, chunk_size, start_from=a)

tasks = dict()

for c in v:
    tag = str(c.mip_level()) + "_" + "_".join([str(i) for i in c.coordinate()])
    tasks.setdefault(c.mip_level(), []).append(tag)

for k in sorted(tasks.keys()):
    f_task = open("{}.txt".format(k), "w")
    for t in tasks[k]:
        f_task.write(t+"\n")
    f_task.close()

