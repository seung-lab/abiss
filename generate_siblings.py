import sys
import chunk_utils as cu

slist = cu.generate_siblings(sys.argv[1])

for s in slist:
    print(s)
