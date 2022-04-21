#!/bin/bash
yell() { echo "$0: $*" >&2;   }
die() { yell "$*"; exit 111;   }
bail() { yell "$*"; exit 0;  }
try() { "$@" || die "cannot $*";   }
try_to_skip() { "$@" && bail "skip $*";  }
just_in_case() { "$@" || true; }
retry() {
    local -r -i max_attempts="$1"; shift
    local -r cmd="$@"
    local -i attempt_num=1
    until $cmd
    do
        if ((attempt_num==max_attempts))
        then
            die "Attempt $attempt_num failed and there are no more attempts left!"
        else
            echo "Attempt $attempt_num failed! Trying again in $attempt_num seconds..."
            sleep $((attempt_num++))
        fi
    done
}


lock_prefix="${AIRFLOW_TMP_DIR}/.cpulock"
ncpus=$(python3 -c "import os; print(len(os.sched_getaffinity(0)))")
cpuid=""
function acquire_cpu_slot() {
    for i in $(python3 -c "exec(\"import os\nfor i in os.sched_getaffinity(0): print(i)\")"); do
        local fn="${lock_prefix}_${i}"
        if [ ! -f $fn ]; then
            cpuid=$i
            echo "Use cpu $i"
            try touch "${lock_prefix}_${i}"
            return
        fi
    done
    die "All ${ncpus} cpus are busy"
}

function release_cpu_slot() {
    local fn="${lock_prefix}_${cpuid}"
    try rm -rf $fn
}

export AIRFLOW_TMP_DIR=${AIRFLOW_TMP_DIR:-"/tmp/airflow"}
export WORKER_HOME=${WORKER_HOME:-"/workspace/seg"}
export CLOUD_VOLUME_CACHE_DIR=${AIRFLOW_TMP_DIR}/cache

SCRIPT_PATH="${WORKER_HOME}/scripts"
BIN_PATH="${WORKER_HOME}/build"
SECRETS=${SECRETS:-"/run/secrets"}
UPLOAD_CMD="gsutil -q -m cp"
UPLOAD_ST_CMD="gsutil -q cp"
DOWNLOAD_CMD="gsutil -m cp"
DOWNLOAD_ST_CMD="gsutil cp"
COMPRESS_CMD="zstd -9 --rm -T0"
COMPRESSST_CMD="zstd -9 --rm"
PARALLEL_CMD="parallel --halt 2 -j ${ncpus}"
COMPRESSED_EXT="zst"
META=""

export PARAM_JSON=$SECRETS/param
#export PARAM_JSON="$SCRIPT_PATH"/param.json

try python3 $SCRIPT_PATH/set_env.py $PARAM_JSON > $SECRETS/config.sh

try source $SECRETS/config.sh
#try . /root/google-cloud-sdk/path.bash.inc

export SEG_MIP=0
export WS_MIP=0
export FILE_PATH=$SCRATCH_PATH/$STAGE
