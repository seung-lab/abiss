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

timeit() {
    local -r task_tag="$1"; shift
    local -r cmd="$@"
    local -i start=$(($(date +%s%N)/1000000))
    try $cmd
    local -i end=$(($(date +%s%N)/1000000))
    local -i duration=end-start
    echo "${STATSD_PREFIX}.segmentation.${STAGE}.${task_tag}.duration:${duration}|ms" > /dev/udp/${STATSD_HOST:-"localhost"}/${STATSD_PORT:-"9125"} || true
}

download_children() {
    local -r json_input="$1"
    local -r fpath="$2"
    try python3 $SCRIPT_PATH/generate_children.py $json_input|tee filelist.txt
    try download_intermediary_files filelist.txt $fpath
    try rm filelist.txt
}

download_neighbors() {
    local -r json_input="$1"
    local -r fpath="$2"
    try python3 $SCRIPT_PATH/generate_neighbours.py $json_input|tee filelist.txt
    try download_intermediary_files filelist.txt $fpath
    try rm filelist.txt
}

download_intermediary_files() {
    local -r flist="$1"
    local -r fpath="$2"
    if [ "$PARANOID" = "1" ]; then
        awk -v fpath=$fpath -v ext="tar.${COMPRESSED_EXT}" '{printf "%s/%s.%s\n%s/%s.data.md5sum\n", fpath, $0, ext, fpath, $0}' $flist | $DOWNLOAD_CMD -I . || die "${FUNCNAME[0]} failed"
    else
        awk -v fpath=$fpath -v ext="tar.${COMPRESSED_EXT}" '{printf "%s/%s.%s\n", fpath, $0, ext}' $flist | $DOWNLOAD_CMD -I . || die "${FUNCNAME[0]} failed"
    fi
    cat $flist | $PARALLEL_CMD "tar axvf {}.tar.${COMPRESSED_EXT}" || die "${FUNCNAME[0]} failed"
    try rm *.tar.${COMPRESSED_EXT}
    if [ "$PARANOID" = "1" ]; then
        cat $flist | $PARALLEL_CMD "md5sum -c --quiet {}.data.md5sum" || die "${FUNCNAME[0]} failed"
    fi
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
SECRETS=${SECRETS:-"${WORKER_HOME}/.cloudvolume/secrets"}
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

if [ ! -f "$SECRETS"/config.sh  ]; then
    try python3 $SCRIPT_PATH/set_env.py $PARAM_JSON > $SECRETS/config.sh
fi

if [ -f "$SECRETS"/s3-secret ] && [ ! -f "$WORKER_HOME"/.aws/credentials ]; then
    try mkdir "$WORKER_HOME"/.aws
    try cp "$SECRETS"/s3-secret "$WORKER_HOME"/.aws/credentials
fi

try source $SECRETS/config.sh
#try . /root/google-cloud-sdk/path.bash.inc

export SEG_MIP=0
export WS_MIP=0
export FILE_PATH=$SCRATCH_PATH/$STAGE

export MIMALLOC_VERBOSE=1
