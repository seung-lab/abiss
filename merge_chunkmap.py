import sys
import os
import chunk_utils as cu

slist = cu.generate_siblings(sys.argv[1])
fnList = ["chunkmap_{}.data".format(s) for s in slist]
cu.merge_files("chunkmap.data", fnList)
filesize = os.path.getsize("chunkmap.data")
print("chunkmap size:", filesize//16)
