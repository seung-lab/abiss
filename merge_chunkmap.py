import sys
import os
import chunk_utils as cu

mip, indices, volume, faces, edges, vertices = cu.generate_siblings(sys.argv[1])
siblings = []
for offset in volume+faces+edges+vertices:
    c = [indices[i]+offset[i] for i in range(3)]
    siblings.append(c)

slist = [cu.chunk_tag(mip, s) for s in siblings]

fnList = ["chunkmap_{}.data".format(s) for s in slist]
cu.merge_files("chunkmap.data", fnList)
filesize = os.path.getsize("chunkmap.data")
print("chunkmap size:", filesize//16)
