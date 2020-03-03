import logging
import numpy as np
import sys
from collections import defaultdict

def generate_remaps():
    dt = [('oid', np.uint64), ('real_bs', np.uint64), ('nid', np.uint64), ('size', np.uint64)]
    data = np.fromfile("matching_faces.data", dtype=dt)
    data[::-1].sort(order="size")

    remap = dict()
    extra_remap = dict()

    for e in data:
        if e['oid'] == e['nid']:
            continue
        if e['oid'] in remap:
            if remap[e['oid']][0] != e['nid']:
                extra_remap[e['nid']] = extra_remap.get(remap[e['oid']][0], remap[e['oid']])
        elif e['nid'] in remap:
            extra_remap[e['nid']] = remap[e['nid']]
            remap[e['oid']] = remap[e['nid']]
        else:
            remap[e['oid']] = extra_remap.get(e['nid'], [e['nid'], e['size']])

    return remap, extra_remap

def generate_extra_sizes(remaps, bs):
    dt = [('oid', np.uint64), ('real_bs', np.uint64), ('nid', np.uint64), ('size', np.uint64)]
    data = np.fromfile("matching_faces.data", dtype=dt)
    visited = set()
    new_sv = []

    for e in data:
        if (e['real_bs'] == 0):
            continue
        if e['oid'] in visited or e['oid'] in bs:
            continue;
        visited.add(e['oid'])
        new_sv.append((remaps.get(e['nid'], [e['nid'], 1])[0], 1))

    np.array(new_sv, dtype=[('nid', np.uint64), ('size', np.uint64)]).tofile("extra_sv_counts.data")

def minmax_remap_pair(pair, remaps):
    npair = [remaps.get(x, [x,1])[0] for x in pair]
    return (min(npair), max(npair))

def write_edges(data, dt, fn):
    output = []
    for k, v in data.items():
        output.append(tuple(list(k)+v))
    np.array(output, dtype=dt).tofile(fn)

def process_edges(tag, remaps):
    d_ie = [("s1", np.uint64), ("s2", np.uint64), ("aff", np.float64), ("area", np.uint64)]

    rg_array = np.fromfile("o_residual_rg.data".format(tag), dtype=d_ie)
    edge_array = np.fromfile("o_incomplete_edges_{}.tmp".format(tag), dtype=d_ie)

    logger.info("rg size: {}, edge size: {}".format(len(rg_array), len(edge_array)))

    merged_edge = dict()
    merged_rg = dict()

    extra_remap = dict()

    for p in [[merged_rg, rg_array],[merged_edge, edge_array]]:
        for e in p[1]:
            if e['s1'] in remaps:
                extra_remap[e['s1']] = remaps[e['s1']]
            if e['s2'] in remaps:
                extra_remap[e['s2']] = remaps[e['s2']]
            key = minmax_remap_pair((e['s1'], e['s2']), remaps)
            if key[0] == key[1]:
                continue
            if key in p[0]:
                p[0][key][0] += e['aff']
                p[0][key][1] += e['area']
                logger.debug("merge {} {} to {} {}, : {} {}".format(e['s1'], e['s2'], key[0], key[1], p[0][key][0], p[0][key][1]))
            else:
                logger.debug("new edges: {}({}) {}({}) {} {}".format(key[0], e['s1'], key[1], e['s2'], e['aff'], e['area']))
                p[0][key] = list(e)[2:]

    logger.info("rg size: {}, edge size: {}".format(len(merged_rg), len(merged_edge)))
    write_edges(merged_rg, d_ie, "residual_rg.data")
    write_edges(merged_edge, d_ie, "incomplete_edges_{}.tmp".format(tag))
    return extra_remap


def process_boundary_supervoxels(tag, remaps):
    boundary_sv = set()
    dt = [('oid', np.uint64), ('real_bs', np.uint64), ('nid', np.uint64), ('size', np.uint64)]
    for i in range(6):
        nbc = set()
        bc = np.fromfile("o_boundary_{}_{}.tmp".format(i,tag), dtype=dt)
        output = dict()
        for s in bc:
            nid = remaps.get(s['nid'], [s['nid'], s['size']])
            if s['oid'] not in output:
                output[s['oid']] = [s['real_bs']]+nid
            elif s['real_bs'] == 1:
                output[s['oid']][0] = 1
            if s['real_bs'] == 1:
                boundary_sv.add(s['oid'])
                nbc.add(nid[0])
        np.array(list(nbc), dtype=np.uint64).tofile("boundary_{}_{}.tmp".format(i,tag))
        np.array([(k,v[0],v[1], v[2]) for k, v in output.items()], dtype=dt).tofile("o_boundary_{}_{}.data".format(i,tag))

    return boundary_sv


def process_sizes(tag, remaps):
    sizes = np.fromfile("o_ongoing_supervoxel_counts.data", dtype=[('sid', np.uint64),('size', np.uint64)])
    for i in range(len(sizes)):
        nid = remaps.get(sizes[i]['sid'], list(sizes[i]))
        if nid[0] != sizes[i]['sid']:
            logger.debug("replace size: {} {}".format(nid, sizes[i]))
        sizes[i]['sid'] = nid[0]

    np.array(sizes).tofile("ongoing_supervoxel_counts.data")


def process_sems(tag, remaps):
    nlabels = 5
    dt = [('sid', np.uint64), ('sem_labels', np.uint64, (nlabels,))]
    sems = np.fromfile("o_ongoing_semantic_labels.data", dtype=dt)
    for i in range(len(sems)):
        nid = remaps.get(sems[i]['sid'], list(sems[i]))
        if nid[0] != sems[i]['sid']:
            logger.debug("replace size: {} {}".format(nid, sems[i]))
        sems[i]['sid'] = nid[0]

    np.array(sems).tofile("ongoing_semantic_labels.data")


def write_extra_remaps(remaps):
    np.array([(k,v[0]) for k, v in remaps.items()], dtype=[('os', np.uint64), ('ns', np.uint64)]).tofile("extra_remaps.data")

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)
tag = sys.argv[1]
remaps, extra_remaps = generate_remaps()
all_remaps = {**remaps, **extra_remaps}
e_remap1 = process_edges(tag, all_remaps)
bs = process_boundary_supervoxels(tag, all_remaps)
generate_extra_sizes(all_remaps, bs)
process_sizes(tag, all_remaps)
process_sems(tag, all_remaps)
write_extra_remaps({**extra_remaps, **e_remap1})
