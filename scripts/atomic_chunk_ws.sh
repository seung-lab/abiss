#!/bin/bash
set -euo pipefail
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1
output_chunk=`basename $1 .json`
output_path=out/ws
try acquire_cpu_slot
try mkdir remap
try mkdir -p ${output_path}/{seg,dend}
try taskset -c $cpuid python3 $SCRIPT_PATH/cut_chunk_ws.py $1
try taskset -c $cpuid $BIN_PATH/ws param.txt aff.raw $WS_HIGH_THRESHOLD $WS_LOW_THRESHOLD $WS_SIZE_THRESHOLD $output_chunk
try touch ongoing_"${output_chunk}".data
try $COMPRESS_CMD seg_"${output_chunk}".data
try mv seg_"${output_chunk}".data."${COMPRESSED_EXT}" ${output_path}/seg/
try mv remap ${output_path}/
if [ "$PARANOID" = "1" ]; then
    try md5sum *_"${output_chunk}".data > ${output_path}/dend/"${output_chunk}".data.md5sum
fi
try tar -cvf - *_"${output_chunk}".data | $COMPRESS_CMD > ${output_path}/dend/"${output_chunk}".tar."${COMPRESSED_EXT}"
retry 10 $UPLOAD_CMD "${output_path}" $IO_SCRATCH_PATH/
try rm -rf ${output_path}
try release_cpu_slot
