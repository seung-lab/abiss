#!/bin/bash
set -euo pipefail
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1
output=`basename $1 .json`

try acquire_cpu_slot
try touch chunkmap.data
retry 10 $DOWNLOAD_CMD $FILE_PATH/seg/seg_"${output}".data."${COMPRESSED_EXT}" seg_"${output}".data."${COMPRESSED_EXT}"
try python3 $SCRIPT_PATH/merge_remaps.py $1
try $COMPRESS_CMD -d seg_"${output}".data."${COMPRESSED_EXT}"
try taskset -c $cpuid $BIN_PATH/ws3 param.txt seg_"${output}".data
try taskset -c $cpuid python3 $SCRIPT_PATH/upload_chunk.py $1 $WS_PATH $WS_MIP
try mv chunkmap.data chunkmap_"${output}".data
retry 10 $COMPRESS_CMD chunkmap_"${output}".data
retry 10 $UPLOAD_CMD chunkmap_"${output}".data."${COMPRESSED_EXT}" "$CHUNKMAP_OUTPUT"/chunkmap_"${output}".data."${COMPRESSED_EXT}"

try release_cpu_slot
