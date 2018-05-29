#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh

output=`basename $1 .json`
echo $output
try python3 $SCRIPT_PATH/generate_filelist.py $1|tee filelist.txt
for fn in $(cat filelist.txt)
do
    try $DOWNLOAD_CMD $FILE_PATH/remap/"${fn}".data."${COMPRESSED_EXT}" .
    try $COMPRESS_CMD -d "${fn}".data."${COMPRESSED_EXT}"
done
try $DOWNLOAD_CMD $FILE_PATH/seg/seg_"${output}".data."${COMPRESSED_EXT}" seg_"${output}".data."${COMPRESSED_EXT}"
try python3 $SCRIPT_PATH/merge_remaps_ws.py $1
try $COMPRESS_CMD -d seg_"${output}".data."${COMPRESSED_EXT}"
try $BIN_PATH/ws3 param.txt seg_"${output}".data
try python3 $SCRIPT_PATH/upload_chunk.py $1 $WS_PATH
