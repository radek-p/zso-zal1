cmake_minimum_required(VERSION 2.8.9)
project(zal1)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -std=c99 -pedantic -Wall -Wextra")

set(LIB_SRC
    loader.h
    loader_private.h
    debug.h
    loader.c
    lazy-resolve.s
)

set_property(SOURCE lazy-resolve.s PROPERTY LANGUAGE C)

# these flags are used in debug.h

# Uncomment to turn on debug messages
#add_definitions("-DDEBUG_MESSAGES")
#add_definitions("-DHITTEST=35")

# target libloader.so:
add_library(loader SHARED ${LIB_SRC})

set(TEST_SRC
    test/run_test.c
    ${LIB_SRC}
)

add_executable(run_test ${TEST_SRC})
