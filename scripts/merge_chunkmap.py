import sys
import os
import chunk_utils as cu
from cloudfiles import CloudFiles
from zstandard import decompress

cf = CloudFiles(os.environ["CHUNKMAP_INPUT"])

mip, indices, volume, faces, edges, vertices = cu.generate_siblings(sys.argv[1])
fn = f"chunkmap_{cu.chunk_tag(mip, indices)}.data.zst"
chunkmap_data = decompress(cf.get(fn))
with open("localmap.data", "wb") as binary_file:
    binary_file.write(chunkmap_data)

siblings = []
for offset in faces+edges+vertices:
    c = [indices[i]+offset[i] for i in range(3)]
    fn = f"chunkmap_{cu.chunk_tag(mip, c)}.data.zst"
    chunkmap_data += decompress(cf.get(fn))

with open("chunkmap.data", "wb") as binary_file:
    binary_file.write(chunkmap_data)
