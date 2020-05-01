#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh

set -euo pipefail

try python3 $SCRIPT_PATH/chunk_volume.py $3 $PARAM_JSON

try python3 $SCRIPT_PATH/generate_batches.py $3 $PARAM_JSON

try cat 0.txt | parallel --verbose $SCRIPT_PATH/run_wrapper.sh . remap_chunk_"$1" {}

