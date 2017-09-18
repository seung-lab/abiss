#!/bin/bash
SCRIPT_PATH="../scripts"
BIN_PATH="../bin"
python $SCRIPT_PATH/cut_chunk.py aff.h5 seg.h5 $1
output=`basename $1 .json`
echo $output
rm -rf $output
mkdir $output
$BIN_PATH/ac param.txt $output
rm *.raw
cp complete_edges_"$output".dat input_rg.dat
$SCRIPT_PATH/agglomerate.sh $output
