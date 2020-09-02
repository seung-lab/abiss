#!/bin/bash
set -euo pipefail
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1
output=`basename $1 .json`
try acquire_cpu_slot
try taskset -c $cpuid python3 $SCRIPT_PATH/cut_chunk_cs.py $1
try taskset -c $cpuid $BIN_PATH/accs param.txt $output
retry 10 $UPLOAD_ST_CMD complete_cs_"${output}".data $FILE_PATH/cs/complete_cs_"${output}".data
try md5sum *_"${output}".data > "${output}".data.md5sum
try tar -cvf - *_"${output}".data | $COMPRESS_CMD > "${output}".tar."${COMPRESSED_EXT}"
retry 10 $UPLOAD_ST_CMD "${output}".tar."${COMPRESSED_EXT}" $FILE_PATH/scratch/"${output}".tar."${COMPRESSED_EXT}"
retry 10 try $UPLOAD_CMD "${output}".data.md5sum $FILE_PATH/scratch/"${output}".data.md5sum
try release_cpu_slot
