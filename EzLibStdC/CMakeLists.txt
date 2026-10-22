cmake_minimum_required(VERSION 3.27)
project(EzLibStdC C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(EzLibStdC STATIC
    src/assert/assert.c
    src/ctype/ctype.c
    src/errno/errno.c
    src/math/math.c
    src/string/string.c
    src/stdlib/stdlib.c
    src/stdio/stdio.c
    src/time/time.c
)

target_include_directories(EzLibStdC PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/c11std
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if (DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY AND NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY STREQUAL "")
    set_target_properties(EzLibStdC PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
else ()
    set_target_properties(EzLibStdC PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
endif ()
