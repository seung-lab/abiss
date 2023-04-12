#!/bin/bash
set -euo pipefail
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1
output_chunk=`basename $1 .json`
output_path=out/agg

just_in_case rm -rf remap
just_in_case rm -rf ${output_path}

if [ "$OVERLAP" = "2"  ]; then
    SCRATCH="scratch2"
else
    SCRATCH="scratch"
fi

try mkdir remap
try touch remap/.nonempty_"$output_chunk".txt

try mkdir -p ${output_path}/{info,$SCRATCH}

for d in $META; do
    just_in_case rm -rf $d
    try mkdir $d
    try touch $d/.nonempty_"$output_chunk".txt
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
    try $BIN_PATH/match_chunks $output_chunk
    try cat extra_remaps.data >> localmap.data
    try cat extra_sv_counts.data >> ongoing_supervoxel_counts.data
fi


if [ "$OVERLAP" = "2" ]; then
    try touch input_rg.data
else
    try mv residual_rg.data input_rg.data
fi

try $BIN_PATH/meme $output_chunk $META
try cat new_edges.data >> input_rg.data
try mv new_edges.data edges_"$output_chunk".data

for i in {0..5}
do
    cat boundary_"$i"_"$output_chunk".data >> frozen.data
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

try $BIN_PATH/split_remap chunk_offset.txt $output_chunk
try $BIN_PATH/assort $output_chunk $META

if [ "$OVERLAP" = "2" ]; then
    for i in {0..5}
    do
        mv o_boundary_"$i"_"$output_chunk".data boundary_"$i"_"$output_chunk".data
    done
fi

try mv done_segments.data ${output_path}/info/info_"$output_chunk".data
try mv done_sem.data ${output_path}/info/semantic_labels_"$output_chunk".data
try mv done_size.data ${output_path}/info/seg_size_"$output_chunk".data
try mv sem_cuts.data ${output_path}/info/sem_rejected_edges_"$output_chunk".log
try mv rejected_edges.log ${output_path}/info/size_rejected_edges_"$output_chunk".log
try mv twig_edges.log ${output_path}/info/twig_edges_"$output_chunk".log
try mv remap ${output_path}

try mv residual_rg.data residual_rg_"$output_chunk".data
try mv ongoing_segments.data ongoing_supervoxel_counts_"$output_chunk".data
try mv ongoing_sem.data ongoing_semantic_labels_"$output_chunk".data
try mv ongoing_size.data ongoing_seg_size_"$output_chunk".data

if [ -n "$META" ]; then
    retry 10 $PARALLEL_CMD $UPLOAD_CMD {} $FILE_PATH/ ::: $META
fi

if [ "$PARANOID" = "1" ]; then
    try md5sum *_"${output_chunk}".data > ${output_path}/"${SCRATCH}"/"${output_chunk}".data.md5sum
fi

try tar -cf - *_"${output_chunk}".data | $COMPRESS_CMD > ${output_path}/"${SCRATCH}"/"${output_chunk}".tar."${COMPRESSED_EXT}"

retry 10 $UPLOAD_CMD "${output_path}" $IO_SCRATCH_PATH/

for fn in $(cat filelist.txt)
do
    try rm -rf $fn
done

try rm -rf remap
try rm -rf ${output_path}

for d in $META; do
    try rm -rf $d
done
