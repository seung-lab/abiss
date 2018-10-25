#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh

output=`basename $1 .json`
echo $output
try python3 $SCRIPT_PATH/generate_filelist.py $1|tee filelist.txt
try cat filelist.txt|$PARALLEL_CMD "$DOWNLOAD_CMD $FILE_PATH/remap/{}.data.${COMPRESSED_EXT} - | $COMPRESS_CMD -d -o {}.data"
try $DOWNLOAD_CMD $FILE_PATH/seg/seg_"${output}".data."${COMPRESSED_EXT}" seg_"${output}".data."${COMPRESSED_EXT}"
try python3 $SCRIPT_PATH/merge_remaps_ws.py $1
try $COMPRESS_CMD -d seg_"${output}".data."${COMPRESSED_EXT}"
try $BIN_PATH/ws3 param.txt seg_"${output}".data
try python3 $SCRIPT_PATH/upload_chunk.py $1 $WS_PATH $WS_MIP
try mv chunkmap.data chunkmap_"${output}".data
try $COMPRESS_CMD chunkmap_"${output}".data
try $UPLOAD_ST_CMD chunkmap_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/remap/chunkmap_"${output}".data."${COMPRESSED_EXT}"
