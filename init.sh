#!/bin/bash
yell() { echo "$0: $*" >&2;   }
die() { yell "$*"; exit 111;   }
bail() { yell "$*"; exit 0;  }
try() { "$@" || die "cannot $*";   }
try_to_skip() { "$@" && bail "skip $*";  }
just_in_case() { "$@" || true; }

lock_prefix="${AIRFLOW_TMP_DIR}/.cpulock"
last_cpu=7
cpuid=""
function acquire_cpu_slot() {
    for i in $(seq 0 $last_cpu); do
        local fn="${lock_prefix}_${i}"
        if [ ! -f $fn ]; then
            cpuid=$i
            echo "Use cpu $i"
            try touch "${lock_prefix}_${i}"
            return
        fi
    done
    die "All cpus are busy"
}

function release_cpu_slot() {
    local fn="${lock_prefix}_${cpuid}"
    try rm -rf $fn
}

SCRIPT_PATH="/root/agg/scripts"
BIN_PATH="/root/agg/build"
UPLOAD_CMD="gsutil -q -m cp"
UPLOAD_ST_CMD="gsutil -q cp"
DOWNLOAD_CMD="gsutil -m cp"
DOWNLOAD_ST_CMD="gsutil cp"
COMPRESS_CMD="zstd -9 --rm -T0"
PARALLEL_CMD="parallel --verbose --halt 2"
COMPRESSED_EXT="zst"
META=""
PARAM_JSON=/root/.cloudvolume/secrets/param
#PARAM_JSON="$SCRIPT_PATH"/param.json

#export AIRFLOW_TMP_DIR="/tmp/airflow"

if [ ! -f "$SCRIPT_PATH"/config.sh ]; then
    try python3 $SCRIPT_PATH/set_env.py $PARAM_JSON > $SCRIPT_PATH/config.sh
fi

try source $SCRIPT_PATH/config.sh
try . /root/google-cloud-sdk/path.bash.inc

