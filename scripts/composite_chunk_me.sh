#!/bin/bash
set -euo pipefail
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1
output=`basename $1 .json`

just_in_case rm -rf remap
just_in_case rm -rf agg_out

if [ "$OVERLAP" = "2"  ]; then
    SCRATCH="scratch2"
else
    SCRATCH="scratch"
fi

try mkdir remap
try touch remap/.nonempty_"$output".txt

try mkdir -p agg_out/{info,$SCRATCH}

for d in $META; do
    just_in_case rm -rf $d
    try mkdir $d
    try touch $d/.nonempty_"$output".txt
done

for fn in $(cat filelist.txt)
do
    just_in_case rm -rf $fn
done

try download_children $1 $FILE_PATH/$SCRATCH

try python3 $SCRIPT_PATH/merge_chunks_me.py $1 $META
try mv ongoing.data localmap.data
if [ "$OVERLAP" = "2"  ]; then
    try mv residual_rg.data o_residual_rg.data
    try mv ongoing_supervoxel_counts.data o_ongoing_supervoxel_counts.data
    try mv ongoing_semantic_labels.data o_ongoing_semantic_labels.data
    try mv ongoing_seg_size.data o_ongoing_seg_size.data
    try $BIN_PATH/match_chunks $output
    try cat extra_remaps.data >> localmap.data
    try cat extra_sv_counts.data >> ongoing_supervoxel_counts.data
fi


if [ "$OVERLAP" = "2" ]; then
    try touch input_rg.data
else
    try mv residual_rg.data input_rg.data
fi

try $BIN_PATH/meme $output $META
try cat new_edges.data >> input_rg.data
try mv new_edges.data edges_"$output".data

for i in {0..5}
do
    cat boundary_"$i"_"$output".data >> frozen.data
done

if [ "$OVERLAP" = "2" ]; then
    try $BIN_PATH/agg_overlap $AGG_THRESHOLD input_rg.data frozen.data ongoing_supervoxel_counts.data
elif [ "$OVERLAP" = "1" ]; then
    try $BIN_PATH/agg_nonoverlap $AGG_THRESHOLD input_rg.data frozen.data ongoing_supervoxel_counts.data
else
    try $BIN_PATH/agg $AGG_THRESHOLD input_rg.data frozen.data ongoing_supervoxel_counts.data
fi

try cat remap.data >> localmap.data

for d in $META; do
    try cat ongoing_"${d}".data >> "${d}".data
done

try $BIN_PATH/split_remap chunk_offset.txt $output
try $BIN_PATH/assort $output $META

if [ "$OVERLAP" = "2" ]; then
    for i in {0..5}
    do
        mv o_boundary_"$i"_"$output".data boundary_"$i"_"$output".data
    done
fi

try mv done_segments.data agg_out/info/info_"$output".data
try mv done_sem.data agg_out/info/semantic_labels_"$output".data
try mv done_size.data agg_out/info/seg_size_"$output".data
try mv sem_cuts.data agg_out/info/sem_rejected_edges_"$output".log
try mv rejected_edges.log agg_out/info/size_rejected_edges_"$output".log
try mv twig_edges.log agg_out/info/twig_edges_"$output".log
try mv remap agg_out

try mv residual_rg.data residual_rg_"$output".data
try mv ongoing_segments.data ongoing_supervoxel_counts_"$output".data
try mv ongoing_sem.data ongoing_semantic_labels_"$output".data
try mv ongoing_size.data ongoing_seg_size_"$output".data

if [ -n "$META" ]; then
    retry 10 $PARALLEL_CMD $UPLOAD_CMD {} $FILE_PATH/ ::: $META
fi

if [ "$PARANOID" = "1" ]; then
    try md5sum *_"${output}".data > agg_out/"${SCRATCH}"/"${output}".data.md5sum
fi

try tar -cf - *_"${output}".data | $COMPRESS_CMD > agg_out/"${SCRATCH}"/"${output}".tar."${COMPRESSED_EXT}"

retry 10 $UPLOAD_CMD "agg_out/*" $FILE_PATH/

for fn in $(cat filelist.txt)
do
    try rm -rf $fn
done

try rm -rf remap
try rm -rf agg_out

for d in $META; do
    try rm -rf $d
done
