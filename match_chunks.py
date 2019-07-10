import logging
import numpy as np
import sys
from collections import defaultdict

logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(__name__)

def generate_remaps():
    dt = [('oid', np.uint64), ('nid', np.uint64), ('size', np.uint64)]
    data = np.fromfile("matching_faces.data", dtype=dt)
    data[::-1].sort(order="size")

    remap = dict()
    extra_remap = dict()

    for e in data:
        if e['oid'] == 72057731611156838 or e['nid'] == 72057731611151754:
            print('map: {} {} {}'.format(e['oid'], e['nid'], e['size']))
            if e['oid'] in remap:
                print("oid in remap")
            if e['nid'] in remap:
                print("nid in remap")
            if e['oid'] in extra_remap:
                print("oid in extra remap")
            if e['nid'] in extra_remap:
                print("oid in extra remap")
        if e['oid'] in remap:
            if remap[e['oid']][0] != e['nid']:
                extra_remap[e['nid']] = extra_remap.get(remap[e['oid']][0], remap[e['oid']])
        elif e['nid'] in remap:
            if e['oid'] == 72057731611156838:
                print("remap {} to {}".format(e['oid'], remap[e['nid']]))
            extra_remap[e['nid']] = remap[e['nid']]
            remap[e['oid']] = remap[e['nid']]
        else:
            remap[e['oid']] = extra_remap.get(e['nid'], [e['nid'], e['size']])

    return remap, extra_remap

def minmax_remap_pair(pair, remaps):
    npair = [remaps.get(x, [x,1])[0] for x in pair]
    return (min(npair), max(npair))

def write_edges(data, dt, fn):
    output = []
    for k, v in data.items():
        output.append(tuple(list(k)+v))
    np.array(output, dtype=dt).tofile(fn)

def process_edges(tag, remaps):
    d_rg = [("s1", np.uint64), ("s2", np.uint64), ("aff", np.float64), ("area", np.uint64),
            ("v1", np.uint64), ("v2", np.uint64), ("local_aff", np.float64), ("local_area", np.uint64)]
    d_ie = [("s1", np.uint64), ("s2", np.uint64), ("aff", np.float64), ("area", np.uint64)]

    rg_array = np.fromfile("o_residual_rg.data".format(tag), dtype=d_rg)
    edge_array = np.fromfile("o_incomplete_edges_{}.tmp".format(tag), dtype=d_ie)

    print("rg size: {}, edge size: {}".format(len(rg_array), len(edge_array)))

    merged_edge = dict()
    merged_rg = dict()

    for p in [[merged_rg, rg_array],[merged_edge, edge_array]]:
        for e in p[1]:
            key = minmax_remap_pair((e['s1'], e['s2']), remaps)
            if key[0] == key[1]:
                continue
            if key in p[0]:
                p[0][key][0] += e['aff']
                p[0][key][1] += e['area']
            else:
                p[0][key] = list(e)[2:]

    print("rg size: {}, edge size: {}".format(len(merged_rg), len(merged_edge)))
    write_edges(merged_rg, d_rg, "residual_rg.data")
    write_edges(merged_edge, d_ie, "incomplete_edges_{}.tmp".format(tag))


def process_boundary_supervoxels(tag, remaps):
    for i in range(6):
        nbc = set()
        bc = np.fromfile("o_boundary_{}_{}.tmp".format(i,tag), dtype=[('oid', np.uint64), ('nid', np.uint64), ('size', np.uint64)])
        output = dict()
        for s in bc:
            nid = remaps.get(s['nid'], [s['nid'], s['size']])
            if s['oid'] not in output:
                output[s['oid']] = nid
            nbc.add(nid[0])
        np.array(list(nbc), dtype=np.uint64).tofile("boundary_{}_{}.tmp".format(i,tag))
        np.array([(k,v[0],v[1]) for k, v in output.items()], dtype=[('oid', np.uint64), ('nid', np.uint64), ('size', np.uint64)]).tofile("o_boundary_{}_{}.data".format(i,tag))


def process_sizes(tag, remaps):
    sizes = np.fromfile("o_ongoing_supervoxel_counts.data", dtype=[('sid', np.uint64),('size', np.uint64)])
    for i in range(len(sizes)):
        nid = remaps.get(sizes[i]['sid'], list(sizes[i]))
        sizes[i]['sid'] = nid[0]

    np.array(sizes).tofile("ongoing_supervoxel_counts.data")


def write_extra_remaps(remaps):
    np.array([(k,v[0]) for k, v in remaps.items()], dtype=[('os', np.uint64), ('ns', np.uint64)]).tofile("extra_remaps.data")

tag = sys.argv[1]
remaps, extra_remaps = generate_remaps()
process_edges(tag, extra_remaps)
process_boundary_supervoxels(tag, extra_remaps)
process_sizes(tag, extra_remaps)
write_extra_remaps(extra_remaps)
