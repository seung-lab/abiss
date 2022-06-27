#!/bin/bash
set -euo pipefail
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1
output=`basename $1 .json`

just_in_case rm -rf remap
try mkdir remap


try download_children $1 $FILE_PATH/dend

try python3 $SCRIPT_PATH/merge_chunks_ws.py $1
#try $BIN_PATH/ws2 param.txt $output >& debug_"${output}".log
try $BIN_PATH/ws2 param.txt $WS_HIGH_THRESHOLD $WS_LOW_THRESHOLD $WS_SIZE_THRESHOLD $output
retry 10 $UPLOAD_CMD done_pre_"${output}".data $FILE_PATH/remap/done_pre_"${output}".data
retry 10 $UPLOAD_CMD done_post_"${output}".data $FILE_PATH/remap/done_post_"${output}".data
try md5sum *_"${output}".data > "${output}".data.md5sum
retry 10 tar -cf - *_"${output}".data | $COMPRESS_CMD > "${output}".tar."${COMPRESSED_EXT}"
retry 10 $UPLOAD_CMD "${output}".tar."${COMPRESSED_EXT}" $FILE_PATH/dend/"${output}".tar."${COMPRESSED_EXT}"
retry 10 $UPLOAD_CMD "${output}".data.md5sum $FILE_PATH/dend/"${output}".data.md5sum

try rm -rf remap
