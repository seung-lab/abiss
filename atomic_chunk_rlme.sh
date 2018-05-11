#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh

output=`basename $1 .json`
echo $output
just_in_case rm -rf $output
try python3 $SCRIPT_PATH/cut_chunk_agg.py $1
try mkdir $output
try $BIN_PATH/ac param.txt $output
try cp complete_edges_"$output".data input_rg.data

for i in {0..5}
do
    cat boundary_"$i"_"$output".data >> frozen.data
done

try $BIN_PATH/agg $AGG_THRESHOLD input_rg.data frozen.data
try mv meta.data meta_"$output".data
try mv residual_rg.data residual_rg_"$output".data
try mv final_rg.data final_rg_"$output".data
try mv mst.data mst_"$output".data
try mv remap.data remap_"$output".data

try $COMPRESS_CMD mst_"${output}".data
try $COMPRESS_CMD remap_"${output}".data
try $COMPRESS_CMD complete_edges_"${output}".data
try $COMPRESS_CMD final_rg_"${output}".data

try $UPLOAD_CMD meta_"${output}".data $FILE_PATH/meta/meta_"${output}".data
try $UPLOAD_CMD mst_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/mst/mst_"${output}".data."${COMPRESSED_EXT}"
try $UPLOAD_CMD remap_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/remap/remap_"${output}".data."${COMPRESSED_EXT}"
try $UPLOAD_CMD complete_edges_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/region_graph/complete_edges_"${output}".data."${COMPRESSED_EXT}"
try $UPLOAD_CMD final_rg_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/region_graph/final_rg_"${output}".data."${COMPRESSED_EXT}"


try find $output -name '*.data' -print > /tmp/test_"${output}".manifest
try tar -cf - --files-from /tmp/test_"${output}".manifest | $COMPRESS_CMD > incomplete_edges_"${output}".tar."${COMPRESSED_EXT}"
try tar -cvf - *_"${output}".data | $COMPRESS_CMD > "${output}".tar."${COMPRESSED_EXT}"
try $UPLOAD_CMD incomplete_edges_"${output}".tar."${COMPRESSED_EXT}" $FILE_PATH/incomplete_edges/incomplete_edges_"${output}".tar."${COMPRESSED_EXT}"
try $UPLOAD_CMD "${output}".tar."${COMPRESSED_EXT}" $FILE_PATH/region_graph/"${output}".tar."${COMPRESSED_EXT}"
try rm -rf $output
