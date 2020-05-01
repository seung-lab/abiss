#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh

set -euo pipefail

try python3 $SCRIPT_PATH/chunk_volume.py $3 $PARAM_JSON

try python3 $SCRIPT_PATH/generate_batches.py $3 $PARAM_JSON

try $SCRIPT_PATH/run_layer.sh 0 atomic_chunk_"$1" $3

for i in $(seq "$2"); do
    try $SCRIPT_PATH/run_layer.sh "$i" composite_chunk_"$1" $3
done

