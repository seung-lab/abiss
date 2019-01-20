#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1

try python3 $SCRIPT_PATH/generate_filelist.py $1|tee filelist.txt
try cat filelist.txt|$PARALLEL_CMD "$DOWNLOAD_CMD $FILE_PATH/remap/{}.data.${COMPRESSED_EXT} - | $COMPRESS_CMD -d -o {}.data"
try python3 $SCRIPT_PATH/cut_chunk_remap.py $1 $WS_PATH
try python3 $SCRIPT_PATH/merge_remaps_ws.py $1 0
try mv seg.raw seg_"${output}".data
try $BIN_PATH/ws3 param.txt seg_"${output}".data
#try python3 $SCRIPT_PATH/ssim.py $1
try python3 $SCRIPT_PATH/upload_chunk.py $1 $SEG_PATH $SEG_MIP

try touch $1.txt
try $UPLOAD_CMD $1.txt $FILE_PATH/done/$1.txt
