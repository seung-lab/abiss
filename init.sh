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

try dos2unix /root/.cloudvolume/secrets/config.sh
try . /root/.cloudvolume/secrets/config.sh
try . /root/google-cloud-sdk/path.bash.inc

