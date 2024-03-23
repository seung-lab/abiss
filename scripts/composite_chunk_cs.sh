#!/bin/bash
set -euo pipefail
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1
output_chunk=`basename $1 .json`

just_in_case rm -rf remap
try mkdir remap

try download_children $1 $FILE_PATH/scratch

try python3 $SCRIPT_PATH/merge_chunks_cs.py $1

try $BIN_PATH/mecs param.txt $output_chunk

retry 10 $UPLOAD_CMD complete_cs_"${output_chunk}".data $FILE_PATH/cs/complete_cs_"${output_chunk}".data
try md5sum *_"${output_chunk}".data > "${output_chunk}".data.md5sum
try tar -cvf - *_"${output_chunk}".data | $COMPRESS_CMD > "${output_chunk}".tar."${COMPRESSED_EXT}"
retry 10 $UPLOAD_CMD "${output_chunk}".tar."${COMPRESSED_EXT}" $FILE_PATH/scratch/"${output_chunk}".tar."${COMPRESSED_EXT}"
retry 10 try $UPLOAD_CMD "${output_chunk}".data.md5sum $FILE_PATH/scratch/"${output_chunk}".data.md5sum

try rm -rf remap
