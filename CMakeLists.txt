cmake_minimum_required(VERSION 2.8)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

include(CheckCXXCompilerFlag)

CHECK_CXX_COMPILER_FLAG("-std=c++1z" COMPILER_SUPPORTS_CXX1Z)
if(COMPILER_SUPPORTS_CXX1Z)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -pthread -fopenmp -std=c++1z -fconcepts")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og -g -std=c++1z")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++1z support. Please use a different C++ compiler.")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Boost COMPONENTS iostreams system REQUIRED)
find_package(Threads)

if(EXTRACT_SIZE)
    add_definitions(-DEXTRACT_SIZE)
endif(EXTRACT_SIZE)

if(EXTRACT_BBOX)
    add_definitions(-DEXTRACT_BBOX)
endif(EXTRACT_BBOX)

if(MST_EDGE)
    add_definitions(-DMST_EDGE)
endif(MST_EDGE)

if(DOUBLE)
    add_definitions(-DDOUBLE)
endif(DOUBLE)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
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
    add_executable(agg_overlap
        src/agg/mean_aggl.cpp)
    add_executable(agg_extra
        src/agg/mean_aggl.cpp)
    target_compile_definitions(agg_overlap PRIVATE OVERLAPPED PRIVATE FINAL PRIVATE EXTRA)
    target_compile_definitions(agg_extra PRIVATE EXTRA)
    target_link_libraries(ws ${Boost_LIBRARIES})
    target_link_libraries(ws2 ${Boost_LIBRARIES} pthread)
    target_link_libraries(ws3 ${Boost_LIBRARIES})
    target_link_libraries(meme ${Boost_LIBRARIES})
    target_link_libraries(acme ${Boost_LIBRARIES})
    target_link_libraries(split_remap pthread)
    target_link_libraries(match_chunks ${Boost_LIBRARIES} pthread)
endif()
