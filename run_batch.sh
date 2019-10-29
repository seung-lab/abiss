#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh

set -euo pipefail

try python3 $SCRIPT_PATH/chunk_volume.py $3 $PARAM_JSON

try python3 $SCRIPT_PATH/generate_batches.py $3 $PARAM_JSON

try cat 0.txt | parallel --delay 1 --verbose --halt 2 $SCRIPT_PATH/run_wrapper.sh . atomic_chunk_"$1" {}

for i in $(seq "$2"); do
    try cat "$i".txt | parallel --delay 1 --verbose --halt 2 $SCRIPT_PATH/run_wrapper.sh . composite_chunk_"$1" {}
done

