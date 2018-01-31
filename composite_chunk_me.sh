#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh

output=`basename $1 .json`
echo $output
just_in_case rm -rf $output
just_in_case rm -rf edges

try python3 $SCRIPT_PATH/generate_children.py $1|tee filelist.txt
for fn in $(cat filelist.txt)
do
    just_in_case rm -rf $fn
    try $DOWNLOAD_CMD $FILE_PATH/scratch/"${fn}".tar."${COMPRESSED_EXT}" .
    try $COMPRESS_CMD -d "${fn}".tar."${COMPRESSED_EXT}"
    try tar -xf "${fn}".tar
    try rm "${fn}".tar
done
try python3 $SCRIPT_PATH/merge_chunks_me.py $1

try mv residual_rg.data input_rg.data
try $BIN_PATH/meme $output
try cat new_edges.data >> input_rg.data
try mv new_edges.data complete_edges_"$output".data

for i in {0..5}
do
    cat boundary_"$i"_"$output".data >> frozen.data
done

try $BIN_PATH/agg $THRESHOLD input_rg.data frozen.data

try mv meta.data meta_"$output".data
try mv mst.data mst_"$output".data
try mv remap.data remap_"$output".data
try mv residual_rg.data residual_rg_"$output".data
try mv final_rg.data final_rg_"$output".data

try $COMPRESS_CMD mst_"${output}".data
try $COMPRESS_CMD remap_"${output}".data
try $COMPRESS_CMD complete_edges_"${output}".data
try $COMPRESS_CMD final_rg_"${output}".data

try $UPLOAD_CMD meta_"${output}".data $FILE_PATH/meta/meta_"${output}".data
try $UPLOAD_CMD mst_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/mst/mst_"${output}".data."${COMPRESSED_EXT}"
try $UPLOAD_CMD remap_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/remap/remap_"${output}".data."${COMPRESSED_EXT}"
try $UPLOAD_CMD complete_edges_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/region_graph/complete_edges_"${output}".data."${COMPRESSED_EXT}"
try $UPLOAD_CMD final_rg_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/region_graph/final_rg_"${output}".data."${COMPRESSED_EXT}"

try tar -cf "${output}".tar *_"${output}".data
try $COMPRESS_CMD "${output}".tar
try $UPLOAD_CMD "${output}".tar."${COMPRESSED_EXT}" $FILE_PATH/scratch/"${output}".tar."${COMPRESSED_EXT}"
for fn in $(cat filelist.txt)
do
    try rm -rf $fn
done
