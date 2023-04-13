#!/bin/bash
set -euo pipefail
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1
output_chunk=`basename $1 .json`
try acquire_cpu_slot
try taskset -c $cpuid python3 $SCRIPT_PATH/cut_chunk_cs.py $1
try taskset -c $cpuid $BIN_PATH/accs param.txt $output_chunk
retry 10 $UPLOAD_CMD complete_cs_"${output_chunk}".data $FILE_PATH/cs/complete_cs_"${output_chunk}".data
try md5sum *_"${output_chunk}".data > "${output_chunk}".data.md5sum
try tar -cvf - *_"${output_chunk}".data | $COMPRESS_CMD > "${output_chunk}".tar."${COMPRESSED_EXT}"
retry 10 $UPLOAD_CMD "${output_chunk}".tar."${COMPRESSED_EXT}" $FILE_PATH/scratch/"${output_chunk}".tar."${COMPRESSED_EXT}"
retry 10 try $UPLOAD_CMD "${output_chunk}".data.md5sum $FILE_PATH/scratch/"${output_chunk}".data.md5sum
try release_cpu_slot
