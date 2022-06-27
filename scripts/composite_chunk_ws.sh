#!/bin/bash
set -euo pipefail
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1
output=`basename $1 .json`

just_in_case rm -rf ws_out
try mkdir -p ws_out/{dend,remap}


try download_children $1 $FILE_PATH/dend

try python3 $SCRIPT_PATH/merge_chunks_ws.py $1
#try $BIN_PATH/ws2 param.txt $output >& debug_"${output}".log
try $BIN_PATH/ws2 param.txt $WS_HIGH_THRESHOLD $WS_LOW_THRESHOLD $WS_SIZE_THRESHOLD $output

try mv done_{pre,post}_"${output}".data ws_out/remap

if [ "$PARANOID" = "1" ]; then
    try md5sum *_"${output}".data > ws_out/dend/"${output}".data.md5sum
fi

try tar -cf - *_"${output}".data | $COMPRESS_CMD > ws_out/dend/"${output}".tar."${COMPRESSED_EXT}"

retry 10 $UPLOAD_CMD -r ws_out/* $FILE_PATH

try rm -rf ws_out
