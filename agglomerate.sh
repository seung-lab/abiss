#!/bin/bash
SCRIPT_PATH="../scripts"
BIN_PATH="../bin"
[ -f frozen.dat  ] && rm frozen.dat
for i in {0..5}
do
    cat boundary_"$i"_"$1".dat >> frozen.dat
done
$BIN_PATH/agg 0.27 input_rg.dat frozen.dat
mv residual_rg.dat residual_rg_"$1".dat
cat mst.dat >> mst_"$1".dat
