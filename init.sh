#!/bin/bash
set -euo pipefail

yell() { echo "$0: $*" >&2;   }
die() { yell "$*"; exit 111;   }
bail() { yell "$*"; exit 0;  }
try() { "$@" || die "cannot $*";   }
just_in_case() { "$@" || true; }

SCRIPT_PATH="/root/agg/scripts"
BIN_PATH="/root/agg/build"
UPLOAD_CMD="gsutil -m cp"
DOWNLOAD_CMD="gsutil -m cp"
COMPRESS_CMD="zstd -9 --rm -T8"
COMPRESSED_EXT="zst"
META=(mst sizes bboxes)
THRESHOLD=0.2

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
