import sys
from chunk_utils import read_inputs
from cut_chunk_common import load_data, cut_data, save_raw_data
import os

def write_metadata(fn, size, boundaries, offset):
    with open(fn, "w") as f:
        f.write(" ".join([str(x) for x in size]))
        f.write("\n")
        f.write(" ".join([str(x) for x in boundaries]))
        f.write("\n")
        f.write(str(offset))

param = read_inputs(sys.argv[1])
bbox = param["bbox"]
boundary_flags = param["boundary_flags"]
offset = param["offset"]

aff = load_data(os.environ['AFF_PATH'])
#aff = numpy.memmap("../aff64.raw", dtype='float64', mode='r', order='F', shape=(2048,2048,256,3))
start_coord = [bbox[i]-1+boundary_flags[i] for i in range(3)]
end_coord = [bbox[i+3]+1-boundary_flags[i+3] for i in range(3)]

aff_cutout = cut_data(aff, start_coord, end_coord, boundary_flags)
save_raw_data("aff.raw", aff_cutout, aff.dtype)
aff.flush_cache()

write_metadata("param.txt", aff_cutout.shape[0:3], boundary_flags, offset)
