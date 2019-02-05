#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh

try python3 $SCRIPT_PATH/chunk_volume.py $PARAM_JSON

try python3 $SCRIPT_PATH/generate_batches.py $3 $PARAM_JSON

try cat 0.txt | parallel --delay 1 --verbose --halt 2 $SCRIPT_PATH/run_wrapper.sh . remap_chunk_"$1" {}

