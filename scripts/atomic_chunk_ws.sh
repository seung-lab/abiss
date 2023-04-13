#!/bin/bash
set -euo pipefail
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1
output_chunk=`basename $1 .json`
try acquire_cpu_slot
try mkdir remap
try mkdir -p ws_out/{seg,dend}
try taskset -c $cpuid python3 $SCRIPT_PATH/cut_chunk_ws.py $1
try taskset -c $cpuid $BIN_PATH/ws param.txt aff.raw $WS_HIGH_THRESHOLD $WS_LOW_THRESHOLD $WS_SIZE_THRESHOLD $output_chunk
try touch ongoing_"${output_chunk}".data
try $COMPRESS_CMD seg_"${output_chunk}".data
try mv seg_"${output_chunk}".data."${COMPRESSED_EXT}" ws_out/seg/
try mv remap ws_out/
if [ "$PARANOID" = "1" ]; then
    try md5sum *_"${output_chunk}".data > ws_out/dend/"${output_chunk}".data.md5sum
fi
try tar -cvf - *_"${output_chunk}".data | $COMPRESS_CMD > ws_out/dend/"${output_chunk}".tar."${COMPRESSED_EXT}"
retry 10 $UPLOAD_CMD "ws_out/*" $FILE_PATH/
try rm -rf ws_out
try release_cpu_slot
