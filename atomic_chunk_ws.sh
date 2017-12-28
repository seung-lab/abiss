#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh

output=`basename $1 .json`
echo $output
try python3 $SCRIPT_PATH/cut_chunk.py aff.h5 $1
try $BIN_PATH/ws param.txt aff.raw $output
try pbzip2 seg_"${output}".data
try gsutil cp seg_"${output}".data.bz2 $FILE_PATH/seg/seg_"${output}".data.bz2
try gsutil cp meta_"${output}".data $FILE_PATH/meta/meta_"${output}".data
try tar --use-compress-prog=pbzip2 -cf "${output}".tar.bz2 *_"${output}".data
try gsutil cp "${output}".tar.bz2 $FILE_PATH/dend/"${output}".tar.bz2
