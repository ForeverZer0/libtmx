cmake_minimum_required(VERSION 3.0.0)
project(
  tmx
  LANGUAGES C
  VERSION 0.1.0)

set(CMAKE_C_STANDARD 99)

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

option(TMX_NO_ZSTD "Enable/disable built-in Zstandard support." OFF)
option(TMX_WARN_UNHANDLED "Enable/disable warnings for unknown document entities." OFF)

set(TMX_SOURCES
    src/cJSON.c
    src/common.c
    src/compression.c
    src/cwalk.c
    src/file.c
    src/error.c
    src/parse.c
    src/parse.h
    src/parse_json.c
    src/parse_xml.c
    src/memory.c
    src/properties.c
    src/xml.c
    src/yxml.c)

if(NOT TMX_NO_ZSTD)
  list(APPEND TMX_SOURCES src/zstd.c)
endif()

add_library(tmx SHARED ${TMX_SOURCES})
target_compile_options(tmx PRIVATE -Wall -g -std=c99)

if(TMX_NO_ZSTD)
  message("[${PROJECT_NAME}] Disabled Zstandard support")
  target_compile_definitions(tmx PRIVATE -DTMX_NOZSTD)
else()
  message("[${PROJECT_NAME}] Compiling with Zstandard support")
endif()

if(TMX_WARN_UNHANDLED)
  message("[${PROJECT_NAME}] Warnings enabled for unhandled elements")
  target_compile_definitions(tmx PRIVATE -DTMX_WARN_UNHANDLED)
else()
  message("[${PROJECT_NAME}] Warnings disabled for unhandled elements")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message("[${PROJECT_NAME}] Debug features enabled")
  target_compile_definitions(tmx PRIVATE -DTMX_DEBUG)
endif()

if(NOT "" STREQUAL ${CMAKE_C_BYTE_ORDER})
  target_compile_definitions(tmx PRIVATE "TMX_${CMAKE_C_BYTE_ORDER}")
endif()

add_subdirectory(examples)
