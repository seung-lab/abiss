#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh

output=`basename $1 .json`
echo $output
just_in_case rm -rf $output
try python3 $SCRIPT_PATH/cut_chunk_agg.py $1
try mkdir $output
try $BIN_PATH/ac param.txt $output
try cp complete_edges_"$output".data input_rg.data

for i in {0..5}
do
    cat boundary_"$i"_"$output".data >> frozen.data
done

try $BIN_PATH/agg $THRESHOLD input_rg.data frozen.data
try mv residual_rg.data residual_rg_"$output".data
try cat mst.data >> mst_"$output".data

try find $output -name '*.data' -print > /tmp/test_"${output}".manifest
try tar --use-compress-prog=pbzip2 -cf incomplete_edges_"${output}".tar.bz2 --files-from /tmp/test_"${output}".manifest
try tar --use-compress-prog=pbzip2 -cvf "${output}".tar.bz2 *_"${output}".data
try gsutil cp incomplete_edges_"${output}".tar.bz2 $FILE_PATH/incomplete_edges/incomplete_edges_"${output}".tar.bz2
try gsutil cp "${output}".tar.bz2 $FILE_PATH/region_graph/"${output}".tar.bz2
try rm -rf $output
