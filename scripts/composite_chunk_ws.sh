#!/bin/bash
set -euo pipefail
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1
output=`basename $1 .json`

just_in_case rm -rf remap
try mkdir remap

try python3 $SCRIPT_PATH/generate_children.py $1|tee filelist.txt

try cat filelist.txt|$PARALLEL_CMD --retries 10 "$DOWNLOAD_ST_CMD $FILE_PATH/dend/{}.tar.${COMPRESSED_EXT} - | $COMPRESS_CMD -d -c - | tar xf -"

try cat filelist.txt|$PARALLEL_CMD --retries 10 "$DOWNLOAD_ST_CMD $FILE_PATH/remap/ongoing_{}.data.${COMPRESSED_EXT} - | $COMPRESS_CMD -d -o ongoing_{}.data"

try cat filelist.txt|$PARALLEL_CMD --retries 10 "$DOWNLOAD_ST_CMD $FILE_PATH/dend/{}.data.md5sum ."

try cat filelist.txt|$PARALLEL_CMD --halt 2 "md5sum -c --quiet {}.data.md5sum"

try python3 $SCRIPT_PATH/merge_chunks_ws.py $1
#try $BIN_PATH/ws2 param.txt $output >& debug_"${output}".log
try $BIN_PATH/ws2 param.txt $WS_HIGH_THRESHOLD $WS_LOW_THRESHOLD $WS_SIZE_THRESHOLD $output
retry 10 $UPLOAD_CMD done_pre_"${output}".data $FILE_PATH/remap/done_pre_"${output}".data
retry 10 $UPLOAD_CMD done_post_"${output}".data $FILE_PATH/remap/done_post_"${output}".data
try md5sum *_"${output}".data > "${output}".data.md5sum
retry 10 $COMPRESS_CMD ongoing_"${output}".data
retry 10 $UPLOAD_CMD ongoing_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/remap/
retry 10 $UPLOAD_CMD meta_"${output}".data $FILE_PATH/meta/meta_"${output}".data
retry 10 tar -cf - *_"${output}".data | $COMPRESS_CMD > "${output}".tar."${COMPRESSED_EXT}"
retry 10 $UPLOAD_CMD "${output}".tar."${COMPRESSED_EXT}" $FILE_PATH/dend/"${output}".tar."${COMPRESSED_EXT}"
retry 10 $UPLOAD_CMD "${output}".data.md5sum $FILE_PATH/dend/"${output}".data.md5sum

try rm -rf remap
