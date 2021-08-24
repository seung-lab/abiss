import sys
import chunk_utils as cu

mip, indices, volume, faces, edges, vertices = cu.generate_siblings(sys.argv[1])

tag = cu.chunk_tag(mip, indices)

param = cu.read_inputs(sys.argv[1])
offset = param["offset"]

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
