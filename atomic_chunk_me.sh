#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh

output=`basename $1 .json`
echo $output

for d in $META; do
    just_in_case rm -rf $d
    try mkdir $d
done

try python3 $SCRIPT_PATH/cut_chunk_agg.py $1
try $BIN_PATH/acme param.txt $output
try cp edges_"$output".data input_rg.data

for i in {0..5}
do
    cat boundary_"$i"_"$output".data >> frozen.data
done

try $BIN_PATH/agg $THRESHOLD input_rg.data frozen.data
try $BIN_PATH/assort $output

try mv meta.data meta_"$output".data
try mv residual_rg.data residual_rg_"$output".data
try mv final_rg.data final_rg_"$output".data
try mv mst.data mst_"$output".data
try mv remap.data remap_"$output".data

try $COMPRESS_CMD mst_"${output}".data
try $COMPRESS_CMD remap_"${output}".data
try $COMPRESS_CMD edges_"${output}".data
try $COMPRESS_CMD final_rg_"${output}".data

for d in $META; do
    if [ "$(ls -A $d)"  ]; then
        try $UPLOAD_CMD -r $d $FILE_PATH/
    fi
done

try $UPLOAD_CMD info_"${output}".txt $FILE_PATH/info/info_"${output}".txt
try $UPLOAD_CMD meta_"${output}".data $FILE_PATH/meta/meta_"${output}".data
try $UPLOAD_CMD mst_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/chunked_mst/mst_"${output}".data."${COMPRESSED_EXT}"
try $UPLOAD_CMD remap_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/remap/remap_"${output}".data."${COMPRESSED_EXT}"
try $UPLOAD_CMD edges_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/region_graph/edges_"${output}".data."${COMPRESSED_EXT}"
try $UPLOAD_CMD final_rg_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/region_graph/final_rg_"${output}".data."${COMPRESSED_EXT}"

try tar -cvf - *_"${output}".data | $COMPRESS_CMD > "${output}".tar."${COMPRESSED_EXT}"
try $UPLOAD_CMD "${output}".tar."${COMPRESSED_EXT}" $FILE_PATH/scratch/"${output}".tar."${COMPRESSED_EXT}"

for d in $META; do
    try rm -rf $d
done
