#!/bin/bash
set -euo pipefail
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1
output=`basename $1 .json`

just_in_case rm -rf remap
try mkdir remap
try touch remap/.nonempty_"$output".txt
for d in $META; do
    just_in_case rm -rf $d
    try mkdir $d
    try touch $d/.nonempty_"$output".txt
done

try python3 $SCRIPT_PATH/generate_children.py $1|tee filelist.txt

for fn in $(cat filelist.txt)
do
    just_in_case rm -rf $fn
done

if [ "$OVERLAP" = "2" ]; then
    try cat filelist.txt | $PARALLEL_CMD --retries 10 "$DOWNLOAD_ST_CMD $FILE_PATH/scratch2/{}.tar.${COMPRESSED_EXT} ."
    try cat filelist.txt | $PARALLEL_CMD --retries 10 "$COMPRESS_CMD -d {}.tar.${COMPRESSED_EXT}"
    try cat filelist.txt | $PARALLEL_CMD --retries 10 "tar xvf {}.tar"
    try cat filelist.txt | $PARALLEL_CMD --retries 10 "rm {}.tar"
    try cat filelist.txt | $PARALLEL_CMD --retries 10 "$DOWNLOAD_ST_CMD $FILE_PATH/scratch2/{}.data.md5sum ."
else
    try cat filelist.txt | $PARALLEL_CMD --retries 10 "$DOWNLOAD_ST_CMD $FILE_PATH/scratch/{}.tar.${COMPRESSED_EXT} ."
    try cat filelist.txt | $PARALLEL_CMD --retries 10 "$COMPRESS_CMD -d {}.tar.${COMPRESSED_EXT}"
    try cat filelist.txt | $PARALLEL_CMD --retries 10 "tar xvf {}.tar"
    try cat filelist.txt | $PARALLEL_CMD --retries 10 "rm {}.tar"
    try cat filelist.txt | $PARALLEL_CMD --retries 10 "$DOWNLOAD_ST_CMD $FILE_PATH/scratch/{}.data.md5sum ."
fi

try cat filelist.txt | $PARALLEL_CMD --halt 2 "md5sum -c --quiet {}.data.md5sum"

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

try mv residual_rg.data residual_rg_"$output".data
try mv done_segments.data info_"$output".data
try mv ongoing_segments.data ongoing_supervoxel_counts_"$output".data
try mv done_sem.data semantic_labels_"$output".data
try mv ongoing_sem.data ongoing_semantic_labels_"$output".data
try mv done_size.data seg_size_"$output".data
try mv ongoing_size.data ongoing_seg_size_"$output".data
try mv rejected_edges.log size_rejected_edges_"$output".log
try mv sem_cuts.data sem_rejected_edges_"$output".log
try mv twig_edges.log twig_edges_"$output".log

if [ -n "$META" ]; then
    retry 10 $PARALLEL_CMD $UPLOAD_CMD -r {} $FILE_PATH/ ::: $META
fi

retry 10 $UPLOAD_CMD info_"${output}".data $FILE_PATH/info/info_"${output}".data
retry 10 $UPLOAD_CMD semantic_labels_"${output}".data $FILE_PATH/info/semantic_labels_"${output}".data
retry 10 $UPLOAD_CMD seg_size_"${output}".data $FILE_PATH/info/seg_size_"${output}".data
retry 10 $UPLOAD_CMD size_rejected_edges_"${output}".log $FILE_PATH/info/size_rejected_edges_"${output}".log
retry 10 $UPLOAD_CMD sem_rejected_edges_"${output}".log $FILE_PATH/info/sem_rejected_edges_"${output}".log
retry 10 $UPLOAD_CMD twig_edges_"${output}".log $FILE_PATH/info/twig_edges_"${output}".log
retry 10 $UPLOAD_CMD remap/done_"${output}".data $FILE_PATH/remap/done_"${output}".data
retry 10 $UPLOAD_CMD remap/size_"${output}".data $FILE_PATH/remap/size_"${output}".data
try md5sum *_"${output}".data > "${output}".data.md5sum
try tar -cf - *_"${output}".data | $COMPRESS_CMD > "${output}".tar."${COMPRESSED_EXT}"
if [ "$OVERLAP" = "2"  ]; then
    retry 10 $UPLOAD_CMD "${output}".tar."${COMPRESSED_EXT}" $FILE_PATH/scratch2/"${output}".tar."${COMPRESSED_EXT}"
    retry 10 $UPLOAD_CMD "${output}".data.md5sum $FILE_PATH/scratch2/"${output}".data.md5sum
else
    retry 10 $UPLOAD_CMD "${output}".tar."${COMPRESSED_EXT}" $FILE_PATH/scratch/"${output}".tar."${COMPRESSED_EXT}"
    retry 10 $UPLOAD_CMD "${output}".data.md5sum $FILE_PATH/scratch/"${output}".data.md5sum
fi

for fn in $(cat filelist.txt)
do
    try rm -rf $fn
done

try rm -rf remap
for d in $META; do
    try rm -rf $d
done

rm *.data
rm *.tmp
rm *.zst
