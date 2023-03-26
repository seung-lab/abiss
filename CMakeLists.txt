cmake_minimum_required(VERSION 3.8)
project(ABISS)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(optionalLibs)


set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -fopenmp")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -Og -g -fopenmp")

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Boost COMPONENTS iostreams system REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)
find_package(absl)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JEMALLOC jemalloc)
pkg_check_modules(MIMALLOC mimalloc)


if(MIMALLOC_FOUND)
    add_definitions(-DUSE_MIMALLOC)
    include_directories(${MIMALLOC_INCLUDE_DIRS})
    set(optionalLibs ${optionalLibs} ${MIMALLOC_LDFLAGS})
    message(STATUS "Use mimalloc as the default allocator.")
elseif(JEMALLOC_FOUND)
    set(optionalLibs ${optionalLibs} ${JEMALLOC_LDFLAGS})
    message(STATUS "Use jemalloc as the default allocator.")
endif()

if(absl_FOUND)
    add_definitions(-DUSE_ABSL_HASHMAP)
    set(optionalLibs ${optionalLibs}
        absl::flat_hash_map
        absl::flat_hash_set
        )
    message(STATUS "Use abseil::flat_hash_map and absl::flat_hash_set")
endif(absl_FOUND)

link_libraries(${optionalLibs})

if(EXTRACT_SIZE)
    add_definitions(-DEXTRACT_SIZE)
endif(EXTRACT_SIZE)

if(EXTRACT_BBOX)
    add_definitions(-DEXTRACT_BBOX)
endif(EXTRACT_BBOX)

if(EXTRACT_COM)
    add_definitions(-DEXTRACT_COM)
endif(EXTRACT_COM)


if(MST_EDGE)
    add_definitions(-DMST_EDGE)
endif(MST_EDGE)

if(DOUBLE)
    add_definitions(-DDOUBLE)
endif(DOUBLE)

add_executable(meme
    src/seg/merge_edges_ME.cpp)
add_executable(acme
    src/seg/atomic_chunk_ME.cpp)
add_executable(assort
    src/seg/assort.cpp)
add_executable(split_remap
    src/seg/split_remap.cpp)
add_executable(match_chunks
    src/seg/match_chunks.cpp)
add_executable(reduce_chunk
    src/seg/reduce_chunk.cpp)
add_executable(size_map
    src/seg/size_map.cpp)
add_executable(evaluate
    src/seg/eval.cpp)
add_executable(ws
    src/ws/atomic_chunk.cpp)
add_executable(ws2
    src/ws/merge_chunks.cpp)
add_executable(ws3
    src/ws/remap.cpp)
add_executable(agg
    src/agg/mean_aggl.cpp)
add_executable(agg_nonoverlap
    src/agg/mean_aggl.cpp)
add_executable(agg_overlap
    src/agg/mean_aggl.cpp)
add_executable(agg_extra
    src/agg/mean_aggl.cpp)
add_executable(accs
    src/seg/contact_surface.cpp)
add_executable(mecs
    src/seg/merge_cs.cpp)
#target_compile_definitions(agg PRIVATE FINAL PRIVATE EXTRA)
#target_compile_definitions(agg_overlap PRIVATE OVERLAPPED PRIVATE FINAL PRIVATE EXTRA)
#target_compile_definitions(agg_extra PRIVATE EXTRA)
target_compile_definitions(agg PRIVATE FINAL)
target_compile_definitions(agg_overlap PRIVATE OVERLAPPED PRIVATE FINAL)
target_link_libraries(ws ${Boost_LIBRARIES})
target_link_libraries(ws2 ${Boost_LIBRARIES} tbb)
target_link_libraries(ws3 ${Boost_LIBRARIES})
target_link_libraries(meme ${Boost_LIBRARIES} tbb)
target_link_libraries(acme ${Boost_LIBRARIES})
target_link_libraries(split_remap tbb)
target_link_libraries(accs ${Boost_LIBRARIES} tbb)
target_link_libraries(mecs ${Boost_LIBRARIES} tbb)
target_link_libraries(match_chunks ${Boost_LIBRARIES} tbb)
target_link_libraries(reduce_chunk ${Boost_LIBRARIES} tbb)
target_link_libraries(agg ${Boost_LIBRARIES} tbb)
target_link_libraries(agg_nonoverlap ${Boost_LIBRARIES} tbb)
target_link_libraries(agg_overlap ${Boost_LIBRARIES} tbb)
target_link_libraries(agg_extra ${Boost_LIBRARIES} tbb)
