#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh

OP=$2
CHUNK=$3
WORK_PATH="$(realpath "$1")"

set +e
try_to_skip $DOWNLOAD_ST_CMD $FILE_PATH/done/"$OP"/"$CHUNK".txt .

set -euo pipefail

echo $WORK_PATH

try pushd $WORK_PATH

if [ ! -f "$CHUNK".json ]; then
    try python3 $SCRIPT_PATH/chunk_volume.py $PARAM_JSON
fi

just_in_case rm -rf "$CHUNK"

try mkdir "$CHUNK"
try pushd "$CHUNK"
try "$SCRIPT_PATH"/"$OP".sh "$WORK_PATH"/"$CHUNK".json
try popd

try touch "${CHUNK}".txt
try $UPLOAD_ST_CMD "${CHUNK}".txt $FILE_PATH/done/"$OP"/"${CHUNK}".txt

try rm -rf "$CHUNK"
try rm "$CHUNK".txt

try popd
