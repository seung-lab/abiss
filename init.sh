#!/bin/bash
yell() { echo "$0: $*" >&2;   }
die() { yell "$*"; exit 111;   }
bail() { yell "$*"; exit 0;  }
try() { "$@" || die "cannot $*";   }
just_in_case() { "$@" || true; }

SCRIPT_PATH="/root/agg/scripts"
BIN_PATH="/root/agg/build"
UPLOAD_CMD="gsutil cp"
DOWNLOAD_CMD="gsutil cp"
THRESHOLD=0.2

#try . /root/google-cloud-sdk/path.bash.inc
just_in_case rm -rf meta
just_in_case rm -rf remap
just_in_case rm -rf *.data
just_in_case rm -rf *.bz2
just_in_case rm -rf *.raw
just_in_case rm -rf *.tmp
just_in_case rm -rf param.txt
