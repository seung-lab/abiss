#!/bin/bash
set -euo pipefail
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1
output_chunk=`basename $1 .json`

try acquire_cpu_slot
try touch chunkmap.data
try taskset -c $cpuid python3 $SCRIPT_PATH/cut_chunk_remap.py $1 $WS_PATH

if [ "$CHUNKED_AGG_OUTPUT" = "1"  ]; then
    try python3 $SCRIPT_PATH/merge_remaps.py $1 $CHUNKED_AGG_OUTPUT
    try cp seg.raw seg_"${output_chunk}".data
    try taskset -c $cpuid $BIN_PATH/ws3 param.txt seg_"${output_chunk}".data
    try taskset -c $cpuid python3 $SCRIPT_PATH/upload_chunk.py $1 $CHUNKED_SEG_PATH $SEG_MIP
    try mv chunkmap.data chunkmap_"${output_chunk}".data
    retry 10 $COMPRESS_CMD chunkmap_"${output_chunk}".data
    retry 10 $UPLOAD_CMD chunkmap_"${output_chunk}".data."${COMPRESSED_EXT}" "$CHUNKMAP_OUTPUT"/chunkmap_"${output_chunk}".data."${COMPRESSED_EXT}"
fi

try python3 $SCRIPT_PATH/merge_remaps.py $1
try python3 $SCRIPT_PATH/merge_size.py $1
try mv seg.raw seg_"${output_chunk}".data
try taskset -c $cpuid $BIN_PATH/ws3 param.txt seg_"${output_chunk}".data
try taskset -c $cpuid $BIN_PATH/size_map seg_"${output_chunk}".data size.data
if [ ! -z ${GT_PATH:-} ]; then
    try taskset -c $cpuid $BIN_PATH/evaluate seg_"${output_chunk}".data gt.raw
    retry 10 $UPLOAD_CMD evaluation.data $FILE_PATH/evaluation/evaluation_"${output_chunk}".data
fi
#try python3 $SCRIPT_PATH/ssim.py $1
try taskset -c $cpuid python3 $SCRIPT_PATH/upload_chunk.py $1 $SEG_PATH $SEG_MIP
try taskset -c $cpuid python3 $SCRIPT_PATH/upload_size.py $1 "$SEG_PATH"/size_map $SEG_MIP

try release_cpu_slot
