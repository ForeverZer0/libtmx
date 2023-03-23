cmake_minimum_required(VERSION 3.0.0)
project(tmx LANGUAGES C VERSION 0.1.0)

set(CMAKE_C_STANDARD 99)

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)






set(TMX_SOURCES 
    src/common.c
    src/compression.c
    src/error.c
    src/layer.c
    src/map.c
    src/memory.c
    src/properties.c
    src/xml.c
    src/yxml.c)

add_library(tmx SHARED ${TMX_SOURCES})
target_compile_options(tmx PRIVATE -Wall -Wno-comment -g -std=c99)

add_compile_definitions(-DTMX_DEBUG)

if (NOT "" STREQUAL ${CMAKE_C_BYTE_ORDER})
target_compile_definitions(tmx PUBLIC "TMX_${CMAKE_C_BYTE_ORDER}")
endif()

add_subdirectory(examples)
