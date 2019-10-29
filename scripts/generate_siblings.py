import sys
import chunk_utils as cu

def gen_in_chunk(tag,idx):
    #print(idx)
    fn = "chunked_rg/in_chunk_{}_{}.data".format(tag, cu.get_chunk_offset(1,idx[0],idx[1],idx[2]))
    open(fn,"w").close()

def gen_between_chunks_and_fake(tag,idx1,idx2):
    offset1 = cu.get_chunk_offset(1,idx1[0],idx1[1],idx1[2])
    offset2 = cu.get_chunk_offset(1,idx2[0],idx2[1],idx2[2])
    if offset1 > offset2:
        offset1,offset2 = offset2,offset1
    fn = "chunked_rg/between_chunks_{}_{}_{}.data".format(tag,offset1,offset2)
    open(fn,"w").close()
    fn = "chunked_rg/fake_{}_{}_{}.data".format(tag,offset1,offset2)
    open(fn,"w").close()

mip, indices, volume, faces, edges, vertices = cu.generate_siblings(sys.argv[1])

tag = cu.chunk_tag(mip, indices)

param = cu.read_inputs(sys.argv[1])
offset = param["offset"]
fn = "remap/done_{}_{}.data".format(tag, offset)
open(fn,"w").close()

#print(volume)
#print(faces)
#print(edges)
#print(vertices)

siblings = []

for offset in volume+faces+edges+vertices:
    c = [indices[i]+offset[i] for i in range(3)]
    siblings.append(c)

for s in [cu.chunk_tag(mip, s) for s in siblings]:
    print(s)

for c in volume+faces+edges+vertices:
    idx = [indices[i]+c[i] for i in range(3)]
    gen_in_chunk(tag,idx)

for f in faces:
    idx1 = indices
    idx2 = [indices[i]+f[i] for i in range(3)]
    gen_between_chunks_and_fake(tag,idx1,idx2)

for e in edges:
    for f in faces:
        if any(e[i] == f[i] for i in range(3)):
            idx1 = [indices[i]+f[i] for i in range(3)]
            idx2 = [indices[i]+e[i] for i in range(3)]
            gen_between_chunks_and_fake(tag,idx1,idx2)

for v in vertices:
    for e in edges:
        idx1 = [indices[i]+v[i] for i in range(3)]
        idx2 = [indices[i]+e[i] for i in range(3)]
        gen_between_chunks_and_fake(tag,idx1,idx2)
