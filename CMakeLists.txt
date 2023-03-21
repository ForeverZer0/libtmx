cmake_minimum_required(VERSION 3.0.0)
project(tmx LANGUAGES C VERSION 0.1.0)

set(CMAKE_C_STANDARD 99)

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

set(TMX_SOURCES 
    src/compression.c
    src/error.c
    src/memory.c
    src/utils.c)

add_library(tmx SHARED ${TMX_SOURCES})
target_compile_options(tmx PRIVATE -Wall -Og)


add_subdirectory(examples)
