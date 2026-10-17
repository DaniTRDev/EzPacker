cmake_minimum_required(VERSION 3.27)
project(chibicc_frontend C)

set(CHIBICC_SOURCES
    tokenize.c
    preprocess.c
    parse.c
    type.c
    strings.c
    hashmap.c
    unicode.c
)

add_library(chibicc_frontend STATIC ${CHIBICC_SOURCES})

target_include_directories(chibicc_frontend PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/shims"
)

target_compile_features(chibicc_frontend PUBLIC c_std_11)

if (MSVC)
    target_compile_definitions(chibicc_frontend PUBLIC
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_DEPRECATE
    )
    target_compile_options(chibicc_frontend PRIVATE
        /wd4018 /wd4244 /wd4267 /wd4996
    )
else ()
    target_compile_options(chibicc_frontend PRIVATE
        -Wno-switch -Wno-unused-parameter -Wno-sign-compare
    )
endif ()
