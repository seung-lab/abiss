#!/bin/bash
set -euo pipefail
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1
output=`basename $1 .json`

just_in_case rm -rf remap
try mkdir remap

try python3 $SCRIPT_PATH/generate_children.py $1|tee filelist.txt

try cat filelist.txt|$PARALLEL_CMD "$DOWNLOAD_ST_CMD $FILE_PATH/dend/{}.tar.${COMPRESSED_EXT} - | $COMPRESS_CMD -d -c - | tar xf -"

try cat filelist.txt|$PARALLEL_CMD "$DOWNLOAD_ST_CMD $FILE_PATH/remap/ongoing_{}.data.${COMPRESSED_EXT} - | $COMPRESS_CMD -d -o ongoing_{}.data"

try python3 $SCRIPT_PATH/merge_chunks_ws.py $1
#try $BIN_PATH/ws2 param.txt $output >& debug_"${output}".log
try $BIN_PATH/ws2 param.txt $WS_HIGH_THRESHOLD $WS_LOW_THRESHOLD $WS_SIZE_THRESHOLD $output
try cat done_remap.txt | $PARALLEL_CMD -X $COMPRESSST_CMD
try cat done_remap.txt | $PARALLEL_CMD -X $UPLOAD_ST_CMD {}.$COMPRESSED_EXT $FILE_PATH/remap/
try $COMPRESS_CMD ongoing_"${output}".data
try $UPLOAD_CMD ongoing_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/remap/
try $UPLOAD_CMD meta_"${output}".data $FILE_PATH/meta/meta_"${output}".data
try tar -cf - *_"${output}".data | $COMPRESS_CMD > "${output}".tar."${COMPRESSED_EXT}"
try $UPLOAD_CMD "${output}".tar."${COMPRESSED_EXT}" $FILE_PATH/dend/"${output}".tar."${COMPRESSED_EXT}"

try rm -rf remap
