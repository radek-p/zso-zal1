cmake_minimum_required(VERSION 2.8.9)
project(zal1)

# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build64")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -pedantic -Wall -Wextra -Wno-format")

set(SOURCE_FILES
    src/readElf.c
    src/loader.h
    src/loader_private.h
    src/loader.c
)

add_executable(read_elf ${SOURCE_FILES})
