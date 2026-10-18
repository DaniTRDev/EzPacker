cmake_minimum_required(VERSION 3.27)
project(yarpgen CXX)

set(YARPGEN_SOURCES
    src/context.cpp
    src/data.cpp
    src/emit_policy.cpp
    src/expr.cpp
    src/gen_policy.cpp
    src/hash.cpp
    src/ir_value.cpp
    src/main.cpp
    src/options.cpp
    src/program.cpp
    src/statistics.cpp
    src/stmt.cpp
    src/type.cpp
    src/utils.cpp
)

add_executable(yarpgen ${YARPGEN_SOURCES})

target_include_directories(yarpgen PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_features(yarpgen PRIVATE cxx_std_17)

target_compile_definitions(yarpgen PRIVATE
    BUILD_VERSION="1adb290f"
    BUILD_DATE="2026:10:17"
    YARPGEN_VERSION_MAJOR="2"
    YARPGEN_VERSION_MINOR="0"
)

if (MSVC)
    target_compile_definitions(yarpgen PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
    target_compile_options(yarpgen PRIVATE
        /wd4267 /wd4244 /wd4996
    )
else ()
    target_compile_options(yarpgen PRIVATE
        -Wno-unused-parameter -Wno-sign-compare
    )
endif ()
