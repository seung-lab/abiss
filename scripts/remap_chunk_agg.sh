#!/bin/bash
set -euo pipefail
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1
output=`basename $1 .json`

try acquire_cpu_slot
try python3 $SCRIPT_PATH/generate_filelist.py $1 1|tee filelist.txt
try cat filelist.txt|$PARALLEL_CMD "$DOWNLOAD_ST_CMD $FILE_PATH/{}.data.${COMPRESSED_EXT} - | $COMPRESS_CMD -d -o {/}.data"
try taskset -c $cpuid python3 $SCRIPT_PATH/cut_chunk_remap.py $1 $WS_PATH
try python3 $SCRIPT_PATH/merge_remaps_ws.py $1 0
try python3 $SCRIPT_PATH/merge_size.py $1
try mv seg.raw seg_"${output}".data
try taskset -c $cpuid $BIN_PATH/ws3 param.txt seg_"${output}".data
try taskset -c $cpuid $BIN_PATH/size_map seg_"${output}".data size.data
#try python3 $SCRIPT_PATH/ssim.py $1
try taskset -c $cpuid python3 $SCRIPT_PATH/upload_chunk.py $1 $SEG_PATH $SEG_MIP
try taskset -c $cpuid python3 $SCRIPT_PATH/upload_size.py $1 "$SEG_PATH"/size_map $SEG_MIP

try release_cpu_slot
