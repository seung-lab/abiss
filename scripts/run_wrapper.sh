#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh

OP=$2
CHUNK=$3
COORD=(${CHUNK//_/ })
STEPS=(${OP//_/ })
WORK_PATH="$(realpath "$1")"
TASK_KEY="${STAGE}_${OP}_${CHUNK}"

set +e
try_to_skip python3 ${SCRIPT_PATH}/check_task_flag.py ${TASK_KEY}

set -euo pipefail

try python3 ${SCRIPT_PATH}/update_task_flag.py ${TASK_KEY} "IN_PROGRESS"

just_in_case cat /root/.cloudvolume/secrets/sysinfo.txt

echo $WORK_PATH

try pushd $WORK_PATH

if [ ! -f "$CHUNK".json ]; then
    try python3 $SCRIPT_PATH/chunk_volume.py $CHUNK $PARAM_JSON
fi

just_in_case rm -rf "$CHUNK"

try mkdir "$CHUNK"
try pushd "$CHUNK"
timeit "${STEPS[0]}.${COORD[0]}" "$SCRIPT_PATH"/"$OP".sh "$WORK_PATH"/"$CHUNK".json
try popd

try python3 ${SCRIPT_PATH}/update_task_flag.py ${TASK_KEY} "DONE"

try rm -rf "$CHUNK"

try popd
