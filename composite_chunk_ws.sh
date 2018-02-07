#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh

output=`basename $1 .json`
echo $output
try python3 $SCRIPT_PATH/generate_children.py $1|tee filelist.txt
for fn in $(cat filelist.txt)
do
    try $DOWNLOAD_CMD $FILE_PATH/dend/$fn.tar."${COMPRESSED_EXT}" .
    try $COMPRESS_CMD -d -c $fn.tar."${COMPRESSED_EXT}"|tar xf -
    just_in_case $DOWNLOAD_CMD $FILE_PATH/remap/ongoing_"${fn}".data."${COMPRESSED_EXT}" .
    just_in_case $COMPRESS_CMD -d ongoing_"${fn}".data."${COMPRESSED_EXT}"
done
try python3 $SCRIPT_PATH/merge_chunks_ws.py $1
#try $BIN_PATH/ws2 param.txt $output >& debug_"${output}".log
try $BIN_PATH/ws2 param.txt $output
try $COMPRESS_CMD remap_"${output}".data
just_in_case $COMPRESS_CMD done_"${output}"_*.data
just_in_case $UPLOAD_CMD done_"${output}"_*.data."${COMPRESSED_EXT}" $FILE_PATH/remap/
try $COMPRESS_CMD ongoing_"${output}".data
try $UPLOAD_CMD ongoing_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/remap/
try $UPLOAD_CMD remap_"${output}".data."${COMPRESSED_EXT}" $FILE_PATH/remap/remap_"${output}".data."${COMPRESSED_EXT}"
try $UPLOAD_CMD meta_"${output}".data $FILE_PATH/meta/meta_"${output}".data
try tar -cf "${output}".tar *_"${output}".data
try $COMPRESS_CMD "${output}".tar
try $UPLOAD_CMD "${output}".tar."${COMPRESSED_EXT}" $FILE_PATH/dend/"${output}".tar."${COMPRESSED_EXT}"
