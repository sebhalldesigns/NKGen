cmake_minimum_required(VERSION 3.10)

project(nkgen)

add_executable(nkgen
    src/main.c
    src/parser/parser.c
    src/header/header.c
    src/source/source.c
    src/translator/translator.c
    src/xml/xml.c
)

target_include_directories(nkgen PRIVATE
    src
)

set(NKGEN_CMAKE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/NKGen.cmake" CACHE STRING "Path to NKGen.cmake" FORCE)