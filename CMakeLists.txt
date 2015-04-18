cmake_minimum_required(VERSION 2.8.9)
project(zal1)

# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build64")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -pedantic -Wall -Wextra -Wno-format")

#set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")

set(LIB_SRC
    src/loader.h
    src/loader_private.h
    src/loader.c
)

set(SOURCE_FILES
    src/readElf.c
    ${LIB_SRC}
)

add_executable(read_elf ${SOURCE_FILES})

add_library(loader SHARED ${LIB_SRC})
