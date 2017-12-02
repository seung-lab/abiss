#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh

output=`basename $1 .json`
echo $output
just_in_case rm -rf $output
just_in_case rm -rf edges
try mkdir $output
try mkdir edges

try python3 $SCRIPT_PATH/generate_children.py $1|tee filelist.txt
for fn in $(cat filelist.txt)
do
    just_in_case rm -rf $fn
    try gsutil cp $DIST/incomplete_edges/incomplete_edges_"${fn}".tar.bz2 .
    try tar jxf incomplete_edges_"${fn}".tar.bz2
    try gsutil cp $DIST/region_graph/"${fn}".tar.bz2 .
    try tar jxf "${fn}".tar.bz2
done
try python3 $SCRIPT_PATH/merge_chunks.py $1

try cp residual_rg_"$output".dat input_rg.dat
try $BIN_PATH/me process_edges.dat
try cat new_edges.dat >> input_rg.dat
try cat new_edges.dat >> complete_edges_"$output".dat

for i in {0..5}
do
    cat boundary_"$i"_"$output".dat >> frozen.dat
done

try $BIN_PATH/agg $THRESHOLD input_rg.dat frozen.dat
try mv residual_rg.dat residual_rg_"$output".dat
try cat mst.dat >> mst_"$output".dat

try find $output -name '*.dat' -print > /tmp/test_"${output}".manifest
try tar --use-compress-prog=pbzip2 -cf incomplete_edges_"${output}".tar.bz2 --files-from /tmp/test_"${output}".manifest
try tar --use-compress-prog=pbzip2 -cf "${output}".tar.bz2 *_"${output}".dat
try gsutil cp incomplete_edges_"${output}".tar.bz2 $DIST/incomplete_edges/incomplete_edges_"${output}".tar.bz2
try gsutil cp "${output}".tar.bz2 $DIST/region_graph/"${output}".tar.bz2
try rm -rf $output
try rm -rf edges
for fn in $(cat filelist.txt)
do
    try rm -rf $fn
done
