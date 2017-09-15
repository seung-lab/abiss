#!/bin/bash
python ../scripts/cut_chunk.py aff.h5 seg.h5 $1
output=`basename $1 .json`
echo $output
rm -rf $output
mkdir $output
../build/ac param.txt $output
rm *.raw
