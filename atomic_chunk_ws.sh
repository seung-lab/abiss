#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1

try mkdir remap
try python3 $SCRIPT_PATH/cut_chunk_ws.py $1
try $BIN_PATH/ws param.txt aff.raw $WS_HIGH_THRESHOLD $WS_LOW_THRESHOLD $WS_SIZE_THRESHOLD $output
try touch remap_"${output}".data
try touch ongoing_"${output}".data
try $COMPRESS_CMD seg_"${output}".data
try $UPLOAD_ST_CMD seg_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/seg/seg_"${output}".data."${COMPRESSED_EXT}"
try $COMPRESS_CMD -r remap
try $UPLOAD_CMD -r remap $FILE_PATH/
try $COMPRESS_CMD remap_"${output}".data
try $UPLOAD_ST_CMD remap_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/remap/remap_"${output}".data."${COMPRESSED_EXT}"
try $COMPRESS_CMD ongoing_"${output}".data
try $UPLOAD_ST_CMD ongoing_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/remap/ongoing_"${output}".data."${COMPRESSED_EXT}"
try $UPLOAD_ST_CMD meta_"${output}".data $FILE_PATH/meta/meta_"${output}".data
try tar -cvf - *_"${output}".data | $COMPRESS_CMD > "${output}".tar."${COMPRESSED_EXT}"
try $UPLOAD_ST_CMD "${output}".tar."${COMPRESSED_EXT}" $FILE_PATH/dend/"${output}".tar."${COMPRESSED_EXT}"
try rm -rf remap
