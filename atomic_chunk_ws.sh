#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh

output=`basename $1 .json`
echo $output
try python3 $SCRIPT_PATH/cut_chunk_ws.py $1
try $BIN_PATH/ws param.txt aff.raw $output
try touch remap_"${output}".data
try touch ongoing_"${output}".data
try $COMPRESS_CMD seg_"${output}".data
try $UPLOAD_CMD seg_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/seg/seg_"${output}".data."${COMPRESSED_EXT}"
try $COMPRESS_CMD remap_"${output}".data
try $UPLOAD_CMD remap_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/remap/remap_"${output}".data."${COMPRESSED_EXT}"
try $COMPRESS_CMD ongoing_"${output}".data
try $UPLOAD_CMD ongoing_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/remap/ongoing_"${output}".data."${COMPRESSED_EXT}"
try $UPLOAD_CMD meta_"${output}".data $FILE_PATH/meta/meta_"${output}".data
try tar -cf "${output}".tar *_"${output}".data
try $COMPRESS_CMD "${output}".tar
try $UPLOAD_CMD "${output}".tar."${COMPRESSED_EXT}" $FILE_PATH/dend/"${output}".tar."${COMPRESSED_EXT}"
