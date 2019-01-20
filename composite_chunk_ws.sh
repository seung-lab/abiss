#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1

just_in_case rm -rf remap
try mkdir remap

try python3 $SCRIPT_PATH/generate_children.py $1|tee filelist.txt

try cat filelist.txt|$PARALLEL_CMD "$DOWNLOAD_CMD $FILE_PATH/dend/{}.tar.${COMPRESSED_EXT} - | $COMPRESS_CMD -d -c - | tar xf -"
try cat filelist.txt|$PARALLEL_CMD "$DOWNLOAD_CMD $FILE_PATH/remap/ongoing_{}.data.${COMPRESSED_EXT} - | $COMPRESS_CMD -d -o ongoing_{}.data"

try python3 $SCRIPT_PATH/merge_chunks_ws.py $1
#try $BIN_PATH/ws2 param.txt $output >& debug_"${output}".log
try $BIN_PATH/ws2 param.txt $WS_HIGH_THRESHOLD $WS_LOW_THRESHOLD $WS_SIZE_THRESHOLD $output
try $COMPRESS_CMD -r remap
try $UPLOAD_CMD -r remap $FILE_PATH/
try $COMPRESS_CMD ongoing_"${output}".data
try $UPLOAD_CMD ongoing_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/remap/
try $UPLOAD_CMD meta_"${output}".data $FILE_PATH/meta/meta_"${output}".data
try tar -cf - *_"${output}".data | $COMPRESS_CMD > "${output}".tar."${COMPRESSED_EXT}"
try $UPLOAD_CMD "${output}".tar."${COMPRESSED_EXT}" $FILE_PATH/dend/"${output}".tar."${COMPRESSED_EXT}"

try touch $1.txt
try $UPLOAD_CMD $1.txt $FILE_PATH/done/$1.txt

try rm -rf remap
