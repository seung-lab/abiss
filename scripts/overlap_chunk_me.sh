#!/bin/bash
set -euo pipefail
INIT_PATH="$(dirname "$0")"
. ${INIT_PATH}/init.sh $1
output_chunk=`basename $1 .json`
output_path=out/agg

try mkdir -p ${output_path}/{info,scratch2}

for fn in $(cat filelist.txt)
do
    just_in_case rm -rf $fn
done

try download_neighbors $1 $FILE_PATH/scratch

try python3 $SCRIPT_PATH/merge_chunks_overlap.py $1 $META
try mv ongoing.data localmap.data

try mv residual_rg.data input_rg.data
try $BIN_PATH/meme $output_chunk $META
try cat new_edges.data >> input_rg.data

for i in {0..5}
do
    cat boundary_"$i"_"$output_chunk".data >> frozen.data
done

try $BIN_PATH/agg_extra $PARAM_JSON input_rg.data frozen.data ongoing_supervoxel_counts.data

try rm boundary_*.data
try rm incomplete_*.data
try rm *.tmp

retry 10 $DOWNLOAD_CMD $FILE_PATH/scratch/${output_chunk}.tar.${COMPRESSED_EXT} .
try tar axvf ${output_chunk}.tar.${COMPRESSED_EXT}
try rm ${output_chunk}.tar.${COMPRESSED_EXT}

if [ "$PARANOID" = "1" ]; then
    try $DOWNLOAD_CMD $FILE_PATH/scratch/${output_chunk}.data.md5sum .
    try md5sum -c --quiet ${output_chunk}.data.md5sum
fi

try $BIN_PATH/reduce_chunk ${output_chunk}

try rm residual_rg*.data

try touch residual_rg_"$output_chunk".data

try cp sem_cuts.data vetoed_edges_"$output_chunk".data
try cat extra_remaps.data >> ongoing_"$output_chunk".data

for i in {0..5}
do
    mv reduced_boundary_"$i"_"$output_chunk".data boundary_"$i"_"$output_chunk".data
done

try mv reduced_edges_"$output_chunk".data incomplete_edges_"$output_chunk".data
try mv reduced_ongoing_supervoxel_counts_"$output_chunk".data ongoing_supervoxel_counts_"$output_chunk".data
try mv reduced_ongoing_semantic_labels_"$output_chunk".data ongoing_semantic_labels_"$output_chunk".data
try mv reduced_ongoing_seg_size_"$output_chunk".data ongoing_seg_size_"$output_chunk".data

try mv done_segments.data ${output_path}/info/info_"$output_chunk"_extra.data
try mv done_sem.data ${output_path}/info/semantic_labels_"$output_chunk"_extra.data
try mv done_size.data ${output_path}/info/seg_size_"$output_chunk"_extra.data
try mv sem_cuts.data ${output_path}/info/sem_rejected_edges_"$output_chunk"_extra.log

if [ "$PARANOID" = "1" ]; then
    try md5sum *_"${output_chunk}".data > ${output_path}/scratch2/"${output_chunk}".data.md5sum
fi
try tar -cf - *_"${output_chunk}".data | $COMPRESS_CMD > ${output_path}/scratch2/"${output_chunk}".tar."${COMPRESSED_EXT}"
retry 10 $UPLOAD_CMD "${output_path}" $IO_SCRATCH_PATH/
try rm -rf ${output_path}
