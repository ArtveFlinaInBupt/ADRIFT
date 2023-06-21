cmake_minimum_required(VERSION 3.25)
project(ADRIFT C)

set(CMAKE_C_STANDARD 23)

set(SRC_DIR src)

include_directories(${SRC_DIR})

add_executable(ADRIFT
    ${SRC_DIR}/main.c
    ${SRC_DIR}/cli/parse.c
    ${SRC_DIR}/external/getopt.c
)
