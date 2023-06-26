cmake_minimum_required(VERSION 3.25)
project(ADRIFT C)

set(CMAKE_C_STANDARD 23)

set(SRC_DIR src)

include_directories(${SRC_DIR})

set(CMAKE_CXX_FLAGS "-W -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Og")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(ADRIFT
    ${SRC_DIR}/main.c
    ${SRC_DIR}/cli/parse.c
    ${SRC_DIR}/external/getopt.c
    ${SRC_DIR}/ds/trie.h
    ${SRC_DIR}/ds/hash.h
    ${SRC_DIR}/ds/hash_map.h
    ${SRC_DIR}/protocol/protocol.c
    ${SRC_DIR}/protocol/cache.c
)
