#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh

[ -f frozen.dat  ] && rm frozen.dat
for i in {0..5}
do
    cat boundary_"$i"_"$1".dat >> frozen.dat
done
try $BIN_PATH/agg 0.2 input_rg.dat frozen.dat
try mv residual_rg.dat residual_rg_"$1".dat
try cat mst.dat >> mst_"$1".dat
