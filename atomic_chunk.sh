#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh

output=`basename $1 .json`
echo $output
just_in_case rm -rf $output
try python3 $SCRIPT_PATH/cut_chunk.py aff.h5 seg.h5 $1
try mkdir $output
try $BIN_PATH/ac param.txt $output
try cp complete_edges_"$output".dat input_rg.dat
try $SCRIPT_PATH/agglomerate.sh $output
try find $output -name '*.dat' -print > /tmp/test_"${output}".manifest
try tar --use-compress-prog=pbzip2 -cf incomplete_edges_"${output}".tar.bz2 --files-from /tmp/test_"${output}".manifest
try tar --use-compress-prog=pbzip2 -cf "${output}".tar.bz2 *_"${output}".dat
try gsutil cp incomplete_edges_"${output}".tar.bz2 $DIST/incomplete_edges/incomplete_edges_"${output}".tar.bz2
try gsutil cp "${output}".tar.bz2 $DIST/region_graph/"${output}".tar.bz2
try rm -rf $output
