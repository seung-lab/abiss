#!/bin/bash
set -euo pipefail
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1
output=`basename $1 .json`

#just_in_case rm -rf remap
#try mkdir remap
#try touch remap/.nonempty_"$output".txt
#for d in $META; do
#    just_in_case rm -rf $d
#    try mkdir $d
#    try touch $d/.nonempty_"$output".txt
#done

try mkdir -p agg_out/{info,scratch2}

for fn in $(cat filelist.txt)
do
    just_in_case rm -rf $fn
done

try download_neighbors $1 $FILE_PATH/scratch

try python3 $SCRIPT_PATH/merge_chunks_overlap.py $1 $META
try mv ongoing.data localmap.data

try mv residual_rg.data input_rg.data
try $BIN_PATH/meme $output $META
try cat new_edges.data >> input_rg.data
#try mv new_edges.data edges_"$output".data

for i in {0..5}
do
    cat boundary_"$i"_"$output".data >> frozen.data
done

try $BIN_PATH/agg_extra $AGG_THRESHOLD input_rg.data frozen.data ongoing_supervoxel_counts.data

try rm boundary_*.data
try rm incomplete_*.data
try rm *.tmp

retry 10 $DOWNLOAD_CMD $FILE_PATH/scratch/${output}.tar.${COMPRESSED_EXT} .
try $COMPRESS_CMD -d ${output}.tar.${COMPRESSED_EXT}
try tar xvf ${output}.tar
try rm ${output}.tar.${COMPRESSED_EXT}
try $DOWNLOAD_ST_CMD $FILE_PATH/scratch/${output}.data.md5sum .
try md5sum -c --quiet ${output}.data.md5sum

try python3 $SCRIPT_PATH/reduce_chunk.py ${output}

try rm residual_rg*.data
#for d in $META; do
#    try cat ongoing_"${d}".data >> "${d}".data
#done
#
#try $BIN_PATH/split_remap chunk_offset.txt $output
#try $BIN_PATH/assort $output $META
#
try cp sem_cuts.data vetoed_edges_"$output".data
try cat extra_remaps.data >> ongoing_"$output".data

for i in {0..5}
do
    mv reduced_boundary_"$i"_"$output".data boundary_"$i"_"$output".data
done

try mv reduced_edges_"$output".data incomplete_edges_"$output".data
try mv reduced_ongoing_supervoxel_counts_"$output".data ongoing_supervoxel_counts_"$output".data
try mv reduced_ongoing_semantic_labels_"$output".data ongoing_semantic_labels_"$output".data
try mv reduced_ongoing_seg_size_"$output".data ongoing_seg_size_"$output".data

try mv done_segments.data agg_out/info/info_"$output"_extra.data
try mv done_sem.data agg_out/info/semantic_labels_"$output".data
try mv done_size.data agg_out/info/seg_size_"$output".data
try mv sem_cuts.data agg_out/info/sem_rejected_edges_"$output".log
#try mv ongoing_segments.data ongoing_supervoxel_counts_"$output".data
#try mv rejected_edges.log rejected_edges_"$output".log
#
#try cat done_remap.txt | $PARALLEL_CMD -X $COMPRESSST_CMD
#
#if [ -n "$META" ]; then
#    try $PARALLEL_CMD $UPLOAD_CMD -r {} $FILE_PATH/ ::: $META
#fi
#
#try $UPLOAD_CMD rejected_edges_"${output}".log $FILE_PATH/info/rejected_edges_"${output}".log
#try cat done_remap.txt | $PARALLEL_CMD -X $UPLOAD_ST_CMD {}.$COMPRESSED_EXT $FILE_PATH/remap/
try md5sum *_"${output}".data > agg_out/scratch2/"${output}".data.md5sum
try tar -cf - *_"${output}".data | $COMPRESS_CMD > agg_out/scratch2/"${output}".tar."${COMPRESSED_EXT}"
retry 10 $UPLOAD_CMD -r "agg_out/*" $FILE_PATH
#try rm *.data
#try rm *.zst
#
#for fn in $(cat filelist.txt)
#do
#    try rm -rf $fn
#done
#
#try rm -rf remap
#for d in $META; do
#    try rm -rf $d
#done
try rm -rf agg_out
