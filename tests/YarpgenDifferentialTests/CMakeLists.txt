cmake_minimum_required(VERSION 3.27)

option(EZPACKER_BUILD_YARPGEN_TESTS "Enable YARPGen-based differential tests" ON)
if(NOT EZPACKER_BUILD_YARPGEN_TESTS)
    return()
endif()

message(STATUS "Configuring EzPacker YARPGen Test Suites...")

# 1. Locate Python 3 Interpreter
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# 2. Intel YARPGen Binary (imported via CMake/Vendor.cmake)
set(YARPGEN_EXECUTABLE "" CACHE FILEPATH "Path to pre-built yarpgen executable")
if(YARPGEN_EXECUTABLE AND EXISTS "${YARPGEN_EXECUTABLE}")
    set(YARPGEN_BIN "${YARPGEN_EXECUTABLE}")
    set(YARPGEN_DEP "")
else()
    set(YARPGEN_BIN "$<TARGET_FILE:yarpgen>")
    set(YARPGEN_DEP yarpgen)
endif()

# 3. Reference Compiler (GCC or Clang) as Source of Truth
if(CMAKE_C_COMPILER)
    set(REFERENCE_C_COMPILER "${CMAKE_C_COMPILER}")
else()
    find_program(REFERENCE_C_COMPILER NAMES gcc clang)
endif()

if(REFERENCE_C_COMPILER)
    message(STATUS "[YarpgenDifferentialTests] Reference compiler (Source of Truth): ${REFERENCE_C_COMPILER}")
    set(REF_COMPILER_FLAG --ref-compiler "${REFERENCE_C_COMPILER}")
else()
    message(STATUS "[YarpgenDifferentialTests] Reference compiler not detected. Running compilation verification; differential execution disabled.")
    set(REF_COMPILER_FLAG --compile-only)
endif()

set(YARPGEN_RUNNER "${CMAKE_CURRENT_SOURCE_DIR}/yarpgen_runner.py")
set(YARPGEN_BULK_BENCH "${CMAKE_CURRENT_SOURCE_DIR}/yarpgen_bulk_bench.py")
set(YARPGEN_STRESS_BENCH "${CMAKE_CURRENT_SOURCE_DIR}/yarpgen_stress_bench.py")
set(YARPGEN_MEGA_STRESS_C "${CMAKE_CURRENT_SOURCE_DIR}/mega_stress.c")
set(YARPGEN_BULK_CORPUS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bulk_corpus")
set(SEEDS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/regression_seeds.txt")

# Target triple setting
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(DEFAULT_TARGET_TRIPLE "x86_64-pc-windows-coff")
else()
    set(DEFAULT_TARGET_TRIPLE "x86_64-unknown-linux-gnu")
endif()

# =========================================================================
# TEST TYPE 1: Bulk File Compilation Suite (Multiple Small/Medium Files)
# =========================================================================
add_custom_target(bulk-ezc
    COMMAND ${Python3_EXECUTABLE} "${YARPGEN_BULK_BENCH}"
        --ezc-bin "$<TARGET_FILE:ezc>"
        --corpus-dir "${YARPGEN_BULK_CORPUS_DIR}"
        --target "${DEFAULT_TARGET_TRIPLE}"
        --opt-levels "O0,O1,O2,Os"
        ${REF_COMPILER_FLAG}
    DEPENDS ezc
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "[Bulk] Running Bulk File Compilation & Verification on 25 Small/Medium Files"
    VERBATIM
)

add_custom_target(bulk-ezc-compile
    COMMAND ${Python3_EXECUTABLE} "${YARPGEN_BULK_BENCH}"
        --ezc-bin "$<TARGET_FILE:ezc>"
        --corpus-dir "${YARPGEN_BULK_CORPUS_DIR}"
        --target "${DEFAULT_TARGET_TRIPLE}"
        --opt-levels "O0,O1,O2,Os"
        --compile-only
    DEPENDS ezc
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "[Bulk] Running Pure Compilation Throughput Benchmark on 25 Small/Medium Files"
    VERBATIM
)

add_custom_target(bulk-ezc-regenerate
    COMMAND ${Python3_EXECUTABLE} "${YARPGEN_BULK_BENCH}"
        --ezc-bin "$<TARGET_FILE:ezc>"
        --yarpgen-bin "${YARPGEN_BIN}"
        --corpus-dir "${YARPGEN_BULK_CORPUS_DIR}"
        --target "${DEFAULT_TARGET_TRIPLE}"
        --regenerate --count 25
    DEPENDS ezc ${YARPGEN_DEP}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "[Bulk] Regenerating Bulk Test Suite of 25 Small/Medium Files via Intel YARPGen"
    VERBATIM
)

# CTest: Bulk Compilation Throughput & Stability
add_test(
    NAME "EzCompilerYarpgen_BulkCompilation"
    COMMAND ${Python3_EXECUTABLE} "${YARPGEN_BULK_BENCH}"
        --ezc-bin "$<TARGET_FILE:ezc>"
        --corpus-dir "${YARPGEN_BULK_CORPUS_DIR}"
        --target "${DEFAULT_TARGET_TRIPLE}"
        --opt-levels "O0,O2"
        --compile-only
)
set_tests_properties("EzCompilerYarpgen_BulkCompilation" PROPERTIES
    TIMEOUT 120
    LABELS "Bulk;Compilation;Yarpgen;EzCompiler"
)

# CTest: Bulk Differential Verification (Source of Truth)
if(REFERENCE_C_COMPILER)
    add_test(
        NAME "EzCompilerYarpgen_BulkDifferential"
        COMMAND ${Python3_EXECUTABLE} "${YARPGEN_BULK_BENCH}"
            --ezc-bin "$<TARGET_FILE:ezc>"
            --corpus-dir "${YARPGEN_BULK_CORPUS_DIR}"
            --ref-compiler "${REFERENCE_C_COMPILER}"
            --target "${DEFAULT_TARGET_TRIPLE}"
            --opt-levels "O0"
    )
    set_tests_properties("EzCompilerYarpgen_BulkDifferential" PROPERTIES
        TIMEOUT 180
        LABELS "Bulk;Differential;Yarpgen;EzCompiler"
    )
endif()

# =========================================================================
# TEST TYPE 2: Single Big File Stress Compilation (+3k LOCs)
# =========================================================================
add_custom_target(stress-ezc
    COMMAND ${Python3_EXECUTABLE} "${YARPGEN_STRESS_BENCH}"
        --ezc-bin "$<TARGET_FILE:ezc>"
        --file "${YARPGEN_MEGA_STRESS_C}"
        --target "${DEFAULT_TARGET_TRIPLE}"
        --opt-level "all"
        ${REF_COMPILER_FLAG}
    DEPENDS ezc
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "[Stress] Running YARPGen Single-File Mega Stress Compilation Benchmark & Pass Profiler"
    VERBATIM
)

add_custom_target(stress-ezc-passes
    COMMAND ${Python3_EXECUTABLE} "${YARPGEN_STRESS_BENCH}"
        --ezc-bin "$<TARGET_FILE:ezc>"
        --file "${YARPGEN_MEGA_STRESS_C}"
        --target "${DEFAULT_TARGET_TRIPLE}"
        --opt-level "all"
        --compile-only
    DEPENDS ezc
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "[Stress] Profiling Compiler Passes on 3,600+ LOC Monolithic Translation Unit"
    VERBATIM
)

add_custom_target(stress-ezc-regenerate
    COMMAND ${Python3_EXECUTABLE} "${YARPGEN_STRESS_BENCH}"
        --ezc-bin "$<TARGET_FILE:ezc>"
        --yarpgen-bin "${YARPGEN_BIN}"
        --file "${YARPGEN_MEGA_STRESS_C}"
        --target "${DEFAULT_TARGET_TRIPLE}"
        --regenerate
    DEPENDS ezc ${YARPGEN_DEP}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "[Stress] Regenerating Single-File Mega Stress Test Suite via Intel YARPGen"
    VERBATIM
)

# CTest: Stress Compilation on Single Big File
add_test(
    NAME "EzCompilerYarpgen_StressCompilation"
    COMMAND ${Python3_EXECUTABLE} "${YARPGEN_STRESS_BENCH}"
        --ezc-bin "$<TARGET_FILE:ezc>"
        --file "${YARPGEN_MEGA_STRESS_C}"
        --target "${DEFAULT_TARGET_TRIPLE}"
        --opt-level "O0"
        --compile-only
)
set_tests_properties("EzCompilerYarpgen_StressCompilation" PROPERTIES
    TIMEOUT 120
    LABELS "Stress;Compilation;Yarpgen;EzCompiler"
)

# CTest: Stress Differential Verification against Reference Compiler
if(REFERENCE_C_COMPILER)
    add_test(
        NAME "EzCompilerYarpgen_StressDifferential"
        COMMAND ${Python3_EXECUTABLE} "${YARPGEN_STRESS_BENCH}"
            --ezc-bin "$<TARGET_FILE:ezc>"
            --file "${YARPGEN_MEGA_STRESS_C}"
            --ref-compiler "${REFERENCE_C_COMPILER}"
            --target "${DEFAULT_TARGET_TRIPLE}"
            --opt-level "O0"
    )
    set_tests_properties("EzCompilerYarpgen_StressDifferential" PROPERTIES
        TIMEOUT 180
        LABELS "Stress;Differential;Yarpgen;EzCompiler"
    )
endif()

# =========================================================================
# Differential Fuzzing Campaign & Regression Seeds
# =========================================================================
if(REFERENCE_C_COMPILER)
    add_custom_target(fuzz-ezc
        COMMAND ${Python3_EXECUTABLE} "${YARPGEN_RUNNER}"
            --ezc-bin "$<TARGET_FILE:ezc>"
            --yarpgen-bin "${YARPGEN_BIN}"
            --ref-compiler "${REFERENCE_C_COMPILER}"
            --target "${DEFAULT_TARGET_TRIPLE}"
            --iterations 25
            --jobs 4
            --work-dir "${CMAKE_CURRENT_BINARY_DIR}/fuzz_work"
            --reproducers-dir "${CMAKE_BINARY_DIR}/yarpgen_reproducers"
        DEPENDS ezc ${YARPGEN_DEP}
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "[Fuzz] Executing 25 parallel YARPGen differential test runs against ${REFERENCE_C_COMPILER}"
        VERBATIM
    )

    if(EXISTS "${SEEDS_FILE}")
        file(STRINGS "${SEEDS_FILE}" REGRESSION_SEEDS)
        foreach(LINE IN LISTS REGRESSION_SEEDS)
            string(STRIP "${LINE}" LINE)
            if(LINE MATCHES "^#" OR LINE STREQUAL "")
                continue()
            endif()

            string(REGEX MATCH "^[0-9]+" SEED "${LINE}")
            if(SEED)
                add_test(
                    NAME "EzCompilerYarpgen_Seed_${SEED}"
                    COMMAND ${Python3_EXECUTABLE} "${YARPGEN_RUNNER}"
                        --ezc-bin "$<TARGET_FILE:ezc>"
                        --yarpgen-bin "${YARPGEN_BIN}"
                        --ref-compiler "${REFERENCE_C_COMPILER}"
                        --target "${DEFAULT_TARGET_TRIPLE}"
                        --seed "${SEED}"
                        --opt-levels "O0,O2"
                        --work-dir "${CMAKE_CURRENT_BINARY_DIR}/seed_${SEED}"
                        --reproducers-dir "${CMAKE_BINARY_DIR}/yarpgen_reproducers"
                )
                set_tests_properties("EzCompilerYarpgen_Seed_${SEED}" PROPERTIES
                    TIMEOUT 60
                    LABELS "Differential;Yarpgen;Regression;EzCompiler"
                )
            endif()
        endforeach()
    endif()
endif()
