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

try cat filelist.txt | $PARALLEL_CMD "$DOWNLOAD_CMD $FILE_PATH/scratch/{}.tar.${COMPRESSED_EXT} - | $COMPRESS_CMD -d -c - | tar xf -"

try python3 $SCRIPT_PATH/merge_chunks_me.py $1 $META
try mv ongoing.data localmap.data

try mv residual_rg.data input_rg.data
try $BIN_PATH/meme $output $META
try cat new_edges.data >> input_rg.data
try mv new_edges.data edges_"$output".data

for i in {0..5}
do
    cat boundary_"$i"_"$output".data >> frozen.data
done

try $BIN_PATH/agg $AGG_THRESHOLD input_rg.data frozen.data ongoing_supervoxel_counts.data

try cat remap.data >> localmap.data

for d in $META; do
    try cat ongoing_"${d}".data >> "${d}".data
done

try $BIN_PATH/split_remap chunk_offset.txt $output
try $BIN_PATH/assort $output $META

try mv meta.data meta_"$output".data
try mv mst.data mst_"$output".data
try mv remap.data remap_"$output".data
try mv residual_rg.data residual_rg_"$output".data
try mv final_rg.data final_rg_"$output".data
try mv done_segments.data info_"$output".data
try mv ongoing_segments.data ongoing_supervoxel_counts_"$output".data
try mv rejected_edges.log rejected_edges_"$output".log

try $COMPRESS_CMD mst_"${output}".data
try $COMPRESS_CMD remap_"${output}".data
try $COMPRESS_CMD edges_"${output}".data
try $COMPRESS_CMD final_rg_"${output}".data
try $COMPRESS_CMD -r remap

if [ -n "$META" ]; then
    try $PARALLEL_CMD $UPLOAD_CMD -r {} $FILE_PATH/ ::: $META
fi

try $UPLOAD_CMD info_"${output}".data $FILE_PATH/info/info_"${output}".data
try $UPLOAD_CMD rejected_edges_"${output}".log $FILE_PATH/info/rejected_edges_"${output}".log
try $UPLOAD_CMD -r remap $FILE_PATH/
try $UPLOAD_CMD meta_"${output}".data $FILE_PATH/meta/meta_"${output}".data
try $UPLOAD_CMD mst_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/chunked_mst/mst_"${output}".data."${COMPRESSED_EXT}"
try $UPLOAD_CMD remap_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/remap/remap_"${output}".data."${COMPRESSED_EXT}"
try $UPLOAD_CMD edges_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/region_graph/edges_"${output}".data."${COMPRESSED_EXT}"
try $UPLOAD_CMD final_rg_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/region_graph/final_rg_"${output}".data."${COMPRESSED_EXT}"

try tar -cf - *_"${output}".data | $COMPRESS_CMD > "${output}".tar."${COMPRESSED_EXT}"
try $UPLOAD_CMD "${output}".tar."${COMPRESSED_EXT}" $FILE_PATH/scratch/"${output}".tar."${COMPRESSED_EXT}"

for fn in $(cat filelist.txt)
do
    try rm -rf $fn
done

try rm -rf remap
for d in $META; do
    try rm -rf $d
done
