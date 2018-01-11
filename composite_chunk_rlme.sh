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
    try $DOWNLOAD_CMD $FILE_PATH/incomplete_edges/incomplete_edges_"${fn}".tar.bz2 .
    try tar --use-compress-prog=pbzip2 -xf incomplete_edges_"${fn}".tar.bz2
    try rm incomplete_edges_"${fn}".tar.bz2
    try $DOWNLOAD_CMD $FILE_PATH/region_graph/"${fn}".tar.bz2 .
    try tar --use-compress-prog=pbzip2 -xf "${fn}".tar.bz2
    try rm "${fn}".tar.bz2
done
try python3 $SCRIPT_PATH/merge_chunks_rlme.py $1

try mv residual_rg.data input_rg.data
try $BIN_PATH/me process_edges.data
try cat new_edges.data >> input_rg.data
try mv new_edges.data complete_edges_"$output".data

for i in {0..5}
do
    cat boundary_"$i"_"$output".data >> frozen.data
done

try $BIN_PATH/agg $THRESHOLD input_rg.data frozen.data

try mv meta.data meta_"$output".data
try mv mst.data mst_"$output".data
try mv remap.data remap_"$output".data
try mv residual_rg.data residual_rg_"$output".data
try mv final_rg.data final_rg_"$output".data

try pbzip2 mst_"${output}".data
try pbzip2 remap_"${output}".data
try pbzip2 complete_edges_"${output}".data
try pbzip2 final_rg_"${output}".data

try $UPLOAD_CMD meta_"${output}".data $FILE_PATH/meta/meta_"${output}".data
try $UPLOAD_CMD mst_"${output}".data.bz2 $FILE_PATH/mst/mst_"${output}".data.bz2
try $UPLOAD_CMD remap_"${output}".data.bz2 $FILE_PATH/remap/remap_"${output}".data.bz2
try $UPLOAD_CMD complete_edges_"${output}".data.bz2 $FILE_PATH/region_graph/complete_edges_"${output}".data.bz2
try $UPLOAD_CMD final_rg_"${output}".data.bz2 $FILE_PATH/region_graph/final_rg_"${output}".data.bz2

try find $output -name '*.data' -print > /tmp/test_"${output}".manifest
try tar --use-compress-prog=pbzip2 -cf incomplete_edges_"${output}".tar.bz2 --files-from /tmp/test_"${output}".manifest
try tar --use-compress-prog=pbzip2 -cf "${output}".tar.bz2 *_"${output}".data
try $UPLOAD_CMD incomplete_edges_"${output}".tar.bz2 $FILE_PATH/incomplete_edges/incomplete_edges_"${output}".tar.bz2
try $UPLOAD_CMD "${output}".tar.bz2 $FILE_PATH/region_graph/"${output}".tar.bz2
try rm -rf $output
try rm -rf edges
for fn in $(cat filelist.txt)
do
    try rm -rf $fn
done
