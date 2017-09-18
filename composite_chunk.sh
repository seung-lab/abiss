#!/bin/bash
SCRIPT_PATH="../scripts"
BIN_PATH="../bin"
output=`basename $1 .json`
echo $output
rm -rf $output
rm -rf edges
mkdir $output
mkdir edges
python $SCRIPT_PATH/merge_chunks.py $1
cp residual_rg_"$output".dat input_rg.dat
$BIN_PATH/me process_edges.dat
cat new_edges.dat >> input_rg.dat
cat new_edges.dat >> complete_edges_"$output".dat
$SCRIPT_PATH/agglomerate.sh $output
