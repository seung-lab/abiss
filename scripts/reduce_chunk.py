import sys
import numpy as np
from cut_chunk_common import load_data
import os
import logging

def consolidate_remaps(remaps):
    nEntries = len(remaps)
    dic = dict()
    for i in range(nEntries):
        e = remaps[nEntries-i-1]
        dic[e['oid']] = dic.get(e['nid'], e['nid'])
    return dic

def load_remaps(fn):
    remaps = np.fromfile(fn, dtype=[('oid', np.uint64), ('nid', np.uint64)])
    return consolidate_remaps(remaps)

def load_sizes(ongoing, done):
    ongoing_counts = {x['sid']:x['s'] for x in np.fromfile(ongoing, dtype=[('sid', np.uint64), ('s', np.uint64)]) }
    done_counts = {x['sid']:x['s'] for x in np.fromfile(done, dtype=[('sid', np.uint64), ('s', np.uint64)]) }
    return ongoing_counts, done_counts

def minmax_remap_pair(pair, remaps):
    npair = [remaps.get(x, x) for x in pair]
    return (min(npair), max(npair))

def reduce_boundaries(tag, remaps, counts):
    reduced_map = dict()
    boundary_sv = set()
    for i in range(6):
        new_sids = []
        data = np.fromfile("boundary_{}_{}.data".format(i, tag), dtype=np.uint64)
        extra_data = np.fromfile("cut_plane_{}_{}.data".format(i, tag), dtype=np.uint64)
        logger.info("frozen sv before: {}".format(len(data)))
        nbs_seeds = set()
        for bs in data:
            boundary_sv.add(bs)
            nbs = remaps.get(bs, bs)
            if bs != nbs:
                reduced_map[bs] = nbs
            if nbs in counts:
                new_sids.append((bs, 1, nbs, counts[nbs]))
                nbs_seeds.add(nbs)
            else:
                logger.warning("face: {}; Impossible: no supervoxel count for {} ({})".format(i, nbs, bs))
        for bs in extra_data:
            nbs = remaps.get(bs, bs)
            if nbs in counts:
                nbs_seeds.add(nbs)
        for k in remaps:
            if remaps[k] in nbs_seeds and k not in reduced_map:
                new_sids.append((k, 0, remaps[k], counts[remaps[k]]))
                nbs_seeds.add(remaps[k])
        logger.info("frozen sv after: {}".format(len(new_sids)))
        np.array(new_sids,dtype=np.uint64).tofile("reduced_boundary_{}_{}.data".format(i, tag))

    return reduced_map, boundary_sv

def reduce_edges(tag, remaps):
    aff = load_data(os.environ['AFF_PATH'],mip=int(os.environ['AFF_MIP']))
    d_ie = [("s1", np.uint64), ("s2", np.uint64), ("aff", aff.dtype), ("area", np.uint64)]
    rg_array = np.fromfile("residual_rg_{}.data".format(tag), dtype=d_ie)
    edge_array = np.fromfile("incomplete_edges_{}.data".format(tag), dtype=d_ie)
    logger.info("Total number of edges: {}".format(len(rg_array)+len(edge_array)))
    merged_edges = dict()
    for l in [rg_array, edge_array]:
        for e in l:
            key = minmax_remap_pair((e['s1'], e['s2']), remaps)
            if key[0] == key[1]:
                continue;
            if key in merged_edges:
                merged_edges[key][0] += e['aff']
                merged_edges[key][1] += e['area']
            else:
                merged_edges[key] = [e['aff'], e['area']]

    logger.info("Total number of reduced edges: {}".format(len(merged_edges)))

    reduced_edges = []
    for k,v in merged_edges.items():
        logger.debug("new edge: {} {} {} {}".format(k[0],k[1],v[0],v[1]))
        reduced_edges.append((k[0], k[1], v[0], v[1]))

    np.array(reduced_edges, dtype=d_ie).tofile("reduced_edges_{}.data".format(tag))

def reduce_counts(tag, remaps, bs):
    dt = [('sid', np.uint64), ('s', np  .uint64)]
    sizes = np.fromfile("ongoing_supervoxel_counts_{}.data".format(tag), dtype=dt)
    reduced_sizes = dict()
    reduced_map = dict()
    for e in sizes:
        if e[0] in bs:
            continue
        sid = remaps.get(e[0], e[0])
        if sid != e[0]:
            reduced_map[e[0]] = sid
        if sid in reduced_sizes:
            reduced_sizes[sid] += e[1]
        else:
            reduced_sizes[sid] = e[1]

    np.array([(k,v) for k, v in reduced_sizes.items()], dtype=dt).tofile("reduced_ongoing_supervoxel_counts_{}.data".format(tag))

    return reduced_map


def reduce_sem(tag, remaps):
    nlabels = 3
    dt = [('sid', np.uint64), ('sem_labels', np.uint64, (nlabels,))]
    sems = np.fromfile("ongoing_semantic_labels_{}.data".format(tag), dtype=dt)
    for e in sems:
        e[0] = remaps.get(e[0], e[0])

    sems.tofile("reduced_ongoing_semantic_labels_{}.data".format(tag))


def reduce_size(tag, remaps):
    nlabels = 3
    dt = [('sid', np.uint64), ('size', np.uint64)]
    sizes = np.fromfile("ongoing_seg_size_{}.data".format(tag), dtype=dt)
    for e in sizes:
        e[0] = remaps.get(e[0], e[0])

    sizes.tofile("reduced_ongoing_seg_size_{}.data".format(tag))


def write_reduced_map(remaps):
    np.array([(k,v) for k, v in remaps.items()], dtype=[('os', np.uint64), ('ns', np.uint64)]).tofile("extra_remaps.data")


logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

remaps = load_remaps("remap.data")

tag = sys.argv[1]
ongoing_counts, done_counts = load_sizes("ongoing_segments.data", "done_segments.data")
counts = {**ongoing_counts, **done_counts}
reduced_map1, bs = reduce_boundaries(tag, remaps, counts)
reduced_map2 = reduce_counts(tag, remaps, bs)
reduce_edges(tag, remaps)
reduce_sem(tag, remaps)
reduce_size(tag, remaps)
write_reduced_map({**reduced_map1, **reduced_map2})
#print(load_segids("0_0_0_0"))
#bs_fname = sys.argv[1]
#bs_array = np.fromfile(bs_fname, dtype=[("chunkid", np.uint64), ("globalid", np.uint64)])


