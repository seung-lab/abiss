#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh

TASK_KEY="${STATSD_PREFIX}_batch_${2}_${3}_${1}"

set +e
try_to_skip python3 ${SCRIPT_PATH}/check_task_flag.py ${TASK_KEY}

set -euo pipefail
try python3 ${SCRIPT_PATH}/update_task_flag.py ${TASK_KEY} "IN_PROGRESS"

if [ "$1" = 0 ]; then
    try cat "$1".txt | $PARALLEL_CMD $SCRIPT_PATH/run_wrapper.sh . "$2" {}
elif [ "$1" = 1 ]; then
    try cat "$1".txt | $PARALLEL_CMD $SCRIPT_PATH/run_wrapper.sh . "$2" {}
elif [ "$1" = 2 ]; then
    try cat "$1".txt | parallel -j 8 --verbose $SCRIPT_PATH/run_wrapper.sh . "$2" {}
else
    try cat "$1".txt | parallel -j 4 --verbose $SCRIPT_PATH/run_wrapper.sh . "$2" {}
fi

try python3 ${SCRIPT_PATH}/update_task_flag.py ${TASK_KEY} "DONE"
