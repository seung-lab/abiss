#!/bin/bash
yell() { echo "$0: $*" >&2;   }
die() { yell "$*"; exit 111;   }
bail() { yell "$*"; exit 0;  }
try() { "$@" || die "cannot $*";   }
try_to_skip() { "$@" && bail "skip $*";  }
just_in_case() { "$@" || true; }

SCRIPT_PATH="/root/agg/scripts"
BIN_PATH="/root/agg/build"
UPLOAD_CMD="gsutil -q -m cp"
UPLOAD_ST_CMD="gsutil -q cp"
DOWNLOAD_CMD="gsutil -m cp"
COMPRESS_CMD="zstd -9 --rm -T0"
PARALLEL_CMD="parallel --verbose --halt 2"
COMPRESSED_EXT="zst"
META=""


try source $SCRIPT_PATH/config.sh

try_to_skip $DOWNLOAD_CMD $FILE_PATH/done/$1.txt .

set -euo pipefail

try dos2unix /root/.cloudvolume/secrets/config.sh
try . /root/.cloudvolume/secrets/config.sh
try . /root/google-cloud-sdk/path.bash.inc

just_in_case rm -rf meta
just_in_case rm -rf remap
just_in_case rm -rf *.data
just_in_case rm -rf *.tar
just_in_case rm -rf *.zst
just_in_case rm -rf *.raw
just_in_case rm -rf *.tmp
just_in_case rm -rf param.txt

output=`basename $1 .json`
echo $output

if [ ! -f $1 ]; then
    try python3 $SCRIPT_PATH/chunk_volume.py
fi
