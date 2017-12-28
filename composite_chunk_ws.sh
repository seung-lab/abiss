#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh

output=`basename $1 .json`
echo $output
try python3 $SCRIPT_PATH/generate_children.py $1|tee filelist.txt
for fn in $(cat filelist.txt)
do
    try gsutil cp $FILE_PATH/dend/$fn.tar.bz2 .
    try tar jxf $fn.tar.bz2
done
try python3 $SCRIPT_PATH/merge_chunks.py $1
#try $BIN_PATH/ws2 param.txt $output >& debug_"${output}".log
try $BIN_PATH/ws2 param.txt $output
try pbzip2 remap_"${output}".data
try gsutil cp remap_"${output}".data.bz2 $FILE_PATH/remap/remap_"${output}".data.bz2
try gsutil cp meta_"${output}".data $FILE_PATH/meta/meta_"${output}".data
try tar --use-compress-prog=pbzip2 -cf "${output}".tar.bz2 *_"${output}".data
try gsutil cp "${output}".tar.bz2 $FILE_PATH/dend/"${output}".tar.bz2
