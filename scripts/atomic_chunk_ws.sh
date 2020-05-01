#!/bin/bash
set -euo pipefail
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1
output=`basename $1 .json`
try acquire_cpu_slot
try mkdir remap
try taskset -c $cpuid python3 $SCRIPT_PATH/cut_chunk_ws.py $1
try taskset -c $cpuid $BIN_PATH/ws param.txt aff.raw $WS_HIGH_THRESHOLD $WS_LOW_THRESHOLD $WS_SIZE_THRESHOLD $output
try touch remap_"${output}".data
try touch ongoing_"${output}".data
try $COMPRESS_CMD seg_"${output}".data
retry 10 $UPLOAD_ST_CMD seg_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/seg/seg_"${output}".data."${COMPRESSED_EXT}"
try $COMPRESS_CMD -r remap
retry 10 $UPLOAD_CMD -r remap $FILE_PATH/
try $COMPRESS_CMD remap_"${output}".data
retry 10 $UPLOAD_ST_CMD remap_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/remap/remap_"${output}".data."${COMPRESSED_EXT}"
try md5sum *_"${output}".data > "${output}".data.md5sum
try $COMPRESS_CMD ongoing_"${output}".data
retry 10 $UPLOAD_ST_CMD ongoing_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/remap/ongoing_"${output}".data."${COMPRESSED_EXT}"
retry 10 $UPLOAD_ST_CMD meta_"${output}".data $FILE_PATH/meta/meta_"${output}".data
try tar -cvf - *_"${output}".data | $COMPRESS_CMD > "${output}".tar."${COMPRESSED_EXT}"
retry 10 $UPLOAD_ST_CMD "${output}".tar."${COMPRESSED_EXT}" $FILE_PATH/dend/"${output}".tar."${COMPRESSED_EXT}"
retry 10 try $UPLOAD_CMD "${output}".data.md5sum $FILE_PATH/dend/"${output}".data.md5sum
try rm -rf remap
try release_cpu_slot
