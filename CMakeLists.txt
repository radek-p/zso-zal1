cmake_minimum_required(VERSION 2.8.9)
project(zal1)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -std=c99 -pedantic -Wall -Wextra")

set(LIB_SRC
    src/loader.h
    src/loader_private.h
    src/loader.c
    src/lazy-resolve.s
)

set_property(SOURCE src/lazy-resolve.s PROPERTY LANGUAGE C)

# these flags are used in debug.h
#add_definitions("-DHITTEST=35")
#add_definitions("-DDEBUG_MESSAGES")

# target libloader.so:
add_library(loader SHARED ${LIB_SRC})


# The testtextrel test and other tests can
# be added to CMake project like this:

# Test library
#set(TEXTRELLIB_SRC
#    tests/textrel.c
#)
#add_library(textrel SHARED ${TEXTRELLIB_SRC})

# Test executable
#set(TEXTREL_SRC
#    tests/testtextrel.c
#)
#add_executable(testtextrel ${TEXTREL_SRC})
#target_link_libraries(testtextrel loader)
