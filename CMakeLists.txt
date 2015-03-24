cmake_minimum_required(VERSION 3.1)
project(zal1)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -pedantic -Wall -Wextra")

set(SOURCE_FILES
    src/readElf.c
)

add_executable(read_elf ${SOURCE_FILES})
