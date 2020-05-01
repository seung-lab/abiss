#!/bin/bash
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh

set +e
try_to_skip $DOWNLOAD_ST_CMD $FILE_PATH/done/batch_"$2"_"$3"_"$1".txt .

set -euo pipefail

if [ "$1" = 0 ]; then
    try cat "$1".txt | parallel --verbose $SCRIPT_PATH/run_wrapper.sh . "$2" {}
elif [ "$1" = 1 ]; then
    try cat "$1".txt | parallel --delay 1 --verbose $SCRIPT_PATH/run_wrapper.sh . "$2" {}
elif [ "$1" = 2 ]; then
    try cat "$1".txt | parallel -j 8 --verbose $SCRIPT_PATH/run_wrapper.sh . "$2" {}
else
    try cat "$1".txt | parallel -j 4 --verbose $SCRIPT_PATH/run_wrapper.sh . "$2" {}
fi

try touch done.txt

try $UPLOAD_ST_CMD done.txt $FILE_PATH/done/batch_"$2"_"$3"_"$1".txt
