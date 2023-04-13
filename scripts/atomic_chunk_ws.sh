#!/bin/bash
set -euo pipefail
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1
output=`basename $1 .json`
try acquire_cpu_slot
try mkdir remap
try mkdir -p ws_out/{seg,dend}
try taskset -c $cpuid python3 $SCRIPT_PATH/cut_chunk_ws.py $1
try taskset -c $cpuid $BIN_PATH/ws param.txt aff.raw $WS_HIGH_THRESHOLD $WS_LOW_THRESHOLD $WS_SIZE_THRESHOLD $output
try touch ongoing_"${output}".data
try $COMPRESS_CMD seg_"${output}".data
try mv seg_"${output}".data."${COMPRESSED_EXT}" ws_out/seg/
try mv remap ws_out/
if [ "$PARANOID" = "1" ]; then
    try md5sum *_"${output}".data > ws_out/dend/"${output}".data.md5sum
fi
try tar -cvf - *_"${output}".data | $COMPRESS_CMD > ws_out/dend/"${output}".tar."${COMPRESSED_EXT}"
retry 10 $UPLOAD_CMD "ws_out/*" $FILE_PATH/
try rm -rf ws_out
try release_cpu_slot
