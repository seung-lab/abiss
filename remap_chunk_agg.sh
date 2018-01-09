#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh

output=`basename $1 .json`
echo $output
try python3 $SCRIPT_PATH/generate_ancestors.py $1 $2|tee filelist.txt
for fn in $(cat filelist.txt)
do
    try $UPLOAD_CMD $FILE_PATH/meta/meta_"${fn}".data .
    try $UPLOAD_CMD $FILE_PATH/remap/remap_"${fn}".data.bz2 .
    try pbzip2 -d remap_"${fn}".data.bz2
done
try python3 $SCRIPT_PATH/cut_chunk_remap.py $1 $WS_PATH
try python3 $SCRIPT_PATH/merge_remaps.py $1 $2
try $BIN_PATH/ws3 param.txt seg.raw
try mv seg.raw seg_"${output}".data
try python3 $SCRIPT_PATH/upload_chunk.py $1 $SEG_PATH
