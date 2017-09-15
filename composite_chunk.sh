#!/bin/bash
output=`basename $1 .json`
echo $output
rm -rf $output
rm -rf edges
mkdir $output
mkdir edges
python ../scripts/merge_chunks.py $1
../build/me process_edges.dat >> complete_edges_"$output".dat
