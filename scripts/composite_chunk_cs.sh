#!/bin/bash
set -euo pipefail
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1
output=`basename $1 .json`

just_in_case rm -rf remap
try mkdir remap

try python3 $SCRIPT_PATH/generate_children.py $1|tee filelist.txt

try cat filelist.txt|$PARALLEL_CMD --retries 10 "$DOWNLOAD_ST_CMD $FILE_PATH/scratch/{}.tar.${COMPRESSED_EXT} - | $COMPRESS_CMD -d -c - | tar xf -"

try cat filelist.txt|$PARALLEL_CMD --retries 10 "$DOWNLOAD_ST_CMD $FILE_PATH/scratch/{}.data.md5sum ."

try cat filelist.txt|$PARALLEL_CMD --halt 2 "md5sum -c --quiet {}.data.md5sum"

try python3 $SCRIPT_PATH/merge_chunks_cs.py $1

try $BIN_PATH/mecs param.txt $output

retry 10 $UPLOAD_ST_CMD complete_cs_"${output}".data $FILE_PATH/cs/complete_cs_"${output}".data
try md5sum *_"${output}".data > "${output}".data.md5sum
try tar -cvf - *_"${output}".data | $COMPRESS_CMD > "${output}".tar."${COMPRESSED_EXT}"
retry 10 $UPLOAD_ST_CMD "${output}".tar."${COMPRESSED_EXT}" $FILE_PATH/scratch/"${output}".tar."${COMPRESSED_EXT}"
retry 10 try $UPLOAD_CMD "${output}".data.md5sum $FILE_PATH/scratch/"${output}".data.md5sum

try rm -rf remap
