#!/bin/bash
set -euo pipefail
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1
output_chunk=`basename $1 .json`
output_path=out/ws

just_in_case rm -rf ${output_path}
try mkdir -p ${output_path}/{dend,remap}


try download_children $1 $FILE_PATH/dend

try python3 $SCRIPT_PATH/merge_chunks_ws.py $1

if [ -f /sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages ]; then
    TOTAL_HUGE_PAGES=$(cat /sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages)
    MIP_LEVEL=$(python3 -c "import json; print(json.load(open('$1'))['mip_level'])")
    TOP_MIP_LEVEL=$(python3 -c "import json; print(json.load(open('$1'))['top_mip_level'])")
    if [ "$MIP_LEVEL" -eq "$TOP_MIP_LEVEL" ]; then
        RESERVE=$TOTAL_HUGE_PAGES
    else
        DIFF=$((TOP_MIP_LEVEL - MIP_LEVEL))
        RESERVE=$((TOTAL_HUGE_PAGES / (1 << DIFF)))
    fi
    export MIMALLOC_RESERVE_HUGE_OS_PAGES=$RESERVE
    echo "RESERVE ${RESERVE} GB HUGE PAGE (mip ${MIP_LEVEL}/${TOP_MIP_LEVEL})"
fi

#try $BIN_PATH/ws2 param.txt $output_chunk >& debug_"${output_chunk}".log
try $BIN_PATH/ws2 param.txt $WS_HIGH_THRESHOLD $WS_LOW_THRESHOLD $WS_SIZE_THRESHOLD $WS_DUST_THRESHOLD $output_chunk

try mv done_{pre,post}_"${output_chunk}".data ${output_path}/remap

if [ "$PARANOID" = "1" ]; then
    try md5sum *_"${output_chunk}".data > ${output_path}/dend/"${output_chunk}".data.md5sum
fi

try tar -cf - *_"${output_chunk}".data | $COMPRESS_CMD > ${output_path}/dend/"${output_chunk}".tar."${COMPRESSED_EXT}"

retry 10 $UPLOAD_CMD "${output_path}" $IO_SCRATCH_PATH/

try rm -rf ${output_path}
