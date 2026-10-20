cmake_minimum_required(VERSION 3.27)

option(EZPACKER_BUILD_YARPGEN_TESTS "Enable YARPGen-based differential tests" ON)
if(NOT EZPACKER_BUILD_YARPGEN_TESTS)
    return()
endif()

message(STATUS "Configuring EzPacker YARPGen Test Suites...")

# 1. Locate Python 3 Interpreter (Optional, non-blocking)
find_package(Python3 COMPONENTS Interpreter)
if(NOT Python3_FOUND)
    message(STATUS "[YarpgenDifferentialTests] Python 3 interpreter not detected; differential script targets skipped.")
    return()
endif()

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

set(YARPGEN_SUITE "${CMAKE_CURRENT_SOURCE_DIR}/yarpgen_suite.py")
set(YARPGEN_MEGA_STRESS_C "${CMAKE_CURRENT_SOURCE_DIR}/mega_stress.c")
set(YARPGEN_MEGA_STRESS_DRIVER "${CMAKE_CURRENT_SOURCE_DIR}/mega_stress_driver.c")
set(YARPGEN_BULK_CORPUS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bulk_corpus")
set(SEEDS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/regression_seeds.txt")

# Target triple setting
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(DEFAULT_TARGET_TRIPLE "x86_64-pc-windows-coff")
else()
    set(DEFAULT_TARGET_TRIPLE "x86_64-unknown-linux-gnu")
endif()

# =========================================================================
# TEST TYPE 1: Bulk File Compilation Suite
# =========================================================================
add_custom_target(bulk-ezc
    COMMAND ${Python3_EXECUTABLE} "${YARPGEN_SUITE}" bulk
        --ezc-bin "$<TARGET_FILE:ezc>"
        --corpus-dir "${YARPGEN_BULK_CORPUS_DIR}"
        --target "${DEFAULT_TARGET_TRIPLE}"
        --opt-levels "O0,O1,O2,Os"
        ${REF_COMPILER_FLAG}
    DEPENDS ezc
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "[Bulk] Running Bulk File Compilation & Verification"
    VERBATIM
)

add_custom_target(bulk-ezc-compile
    COMMAND ${Python3_EXECUTABLE} "${YARPGEN_SUITE}" bulk
        --ezc-bin "$<TARGET_FILE:ezc>"
        --corpus-dir "${YARPGEN_BULK_CORPUS_DIR}"
        --target "${DEFAULT_TARGET_TRIPLE}"
        --opt-levels "O0,O1,O2,Os"
        --compile-only
    DEPENDS ezc
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "[Bulk] Running Pure Compilation Throughput Benchmark"
    VERBATIM
)

# CTest: Bulk Compilation Throughput & Stability
add_test(
    NAME "EzCompilerYarpgen_BulkCompilation"
    COMMAND ${Python3_EXECUTABLE} "${YARPGEN_SUITE}" bulk
        --ezc-bin "$<TARGET_FILE:ezc>"
        --corpus-dir "${YARPGEN_BULK_CORPUS_DIR}"
        --target "${DEFAULT_TARGET_TRIPLE}"
        --opt-levels "O0,O2"
        --compile-only
)
set_tests_properties("EzCompilerYarpgen_BulkCompilation" PROPERTIES
    RUN_SERIAL TRUE
    TIMEOUT 120
    LABELS "Bulk;Compilation;Yarpgen;EzCompiler"
)

# CTest: Bulk Differential Verification
if(REFERENCE_C_COMPILER)
    add_test(
        NAME "EzCompilerYarpgen_BulkDifferential"
        COMMAND ${Python3_EXECUTABLE} "${YARPGEN_SUITE}" bulk
            --ezc-bin "$<TARGET_FILE:ezc>"
            --corpus-dir "${YARPGEN_BULK_CORPUS_DIR}"
            --ref-compiler "${REFERENCE_C_COMPILER}"
            --target "${DEFAULT_TARGET_TRIPLE}"
            --opt-levels "O0"
    )
    set_tests_properties("EzCompilerYarpgen_BulkDifferential" PROPERTIES
        RUN_SERIAL TRUE
        TIMEOUT 180
        LABELS "Bulk;Differential;Yarpgen;EzCompiler"
    )
endif()

# =========================================================================
# TEST TYPE 2: Single Big File Stress Compilation
# =========================================================================
add_custom_target(stress-ezc
    COMMAND ${Python3_EXECUTABLE} "${YARPGEN_SUITE}" stress
        --ezc-bin "$<TARGET_FILE:ezc>"
        --file "${YARPGEN_MEGA_STRESS_C}"
        --driver "${YARPGEN_MEGA_STRESS_DRIVER}"
        --target "${DEFAULT_TARGET_TRIPLE}"
        --opt-level "all"
        ${REF_COMPILER_FLAG}
    DEPENDS ezc
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "[Stress] Running YARPGen Single-File Mega Stress Compilation Benchmark & Pass Profiler"
    VERBATIM
)

add_custom_target(stress-ezc-passes
    COMMAND ${Python3_EXECUTABLE} "${YARPGEN_SUITE}" stress
        --ezc-bin "$<TARGET_FILE:ezc>"
        --file "${YARPGEN_MEGA_STRESS_C}"
        --target "${DEFAULT_TARGET_TRIPLE}"
        --opt-level "all"
        --compile-only
        -v
    DEPENDS ezc
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "[Stress] Profiling Compiler Passes on Monolithic Translation Unit"
    VERBATIM
)

# CTest: Stress Compilation on Single Big File
add_test(
    NAME "EzCompilerYarpgen_StressCompilation"
    COMMAND ${Python3_EXECUTABLE} "${YARPGEN_SUITE}" stress
        --ezc-bin "$<TARGET_FILE:ezc>"
        --file "${YARPGEN_MEGA_STRESS_C}"
        --target "${DEFAULT_TARGET_TRIPLE}"
        --opt-level "O0"
        --compile-only
)
set_tests_properties("EzCompilerYarpgen_StressCompilation" PROPERTIES
    RUN_SERIAL TRUE
    TIMEOUT 120
    LABELS "Stress;Compilation;Yarpgen;EzCompiler"
)

# CTest: Stress Differential Verification against Reference Compiler
if(REFERENCE_C_COMPILER)
    add_test(
        NAME "EzCompilerYarpgen_StressDifferential"
        COMMAND ${Python3_EXECUTABLE} "${YARPGEN_SUITE}" stress
            --ezc-bin "$<TARGET_FILE:ezc>"
            --file "${YARPGEN_MEGA_STRESS_C}"
            --driver "${YARPGEN_MEGA_STRESS_DRIVER}"
            --ref-compiler "${REFERENCE_C_COMPILER}"
            --target "${DEFAULT_TARGET_TRIPLE}"
            --opt-level "O0"
    )
    set_tests_properties("EzCompilerYarpgen_StressDifferential" PROPERTIES
        RUN_SERIAL TRUE
        TIMEOUT 180
        LABELS "Stress;Differential;Yarpgen;EzCompiler"
    )
endif()

# =========================================================================
# Differential Fuzzing Campaign & Regression Seeds
# =========================================================================
if(REFERENCE_C_COMPILER)
    add_custom_target(fuzz-ezc
        COMMAND ${Python3_EXECUTABLE} "${YARPGEN_SUITE}" fuzz
            --ezc-bin "$<TARGET_FILE:ezc>"
            --yarpgen-bin "${YARPGEN_BIN}"
            --ref-compiler "${REFERENCE_C_COMPILER}"
            --target "${DEFAULT_TARGET_TRIPLE}"
            --iterations 25
            --work-dir "${CMAKE_CURRENT_BINARY_DIR}/fuzz_work"
            --reproducers-dir "${CMAKE_BINARY_DIR}/yarpgen_reproducers"
        DEPENDS ezc ${YARPGEN_DEP}
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "[Fuzz] Executing 25 YARPGen differential test runs against ${REFERENCE_C_COMPILER}"
        VERBATIM
    )

    if(EXISTS "${SEEDS_FILE}")
        add_custom_target(seeds-ezc
            COMMAND ${Python3_EXECUTABLE} "${YARPGEN_SUITE}" seeds
                --ezc-bin "$<TARGET_FILE:ezc>"
                --yarpgen-bin "${YARPGEN_BIN}"
                --ref-compiler "${REFERENCE_C_COMPILER}"
                --target "${DEFAULT_TARGET_TRIPLE}"
                --seeds-file "${SEEDS_FILE}"
                --work-dir "${CMAKE_CURRENT_BINARY_DIR}/seeds_work"
                --reproducers-dir "${CMAKE_BINARY_DIR}/yarpgen_reproducers"
            DEPENDS ezc ${YARPGEN_DEP}
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            COMMENT "[Seeds] Verifying curated regression seeds against ${REFERENCE_C_COMPILER}"
            VERBATIM
        )

        add_test(
            NAME "EzCompilerYarpgen_RegressionSeeds"
            COMMAND ${Python3_EXECUTABLE} "${YARPGEN_SUITE}" seeds
                --ezc-bin "$<TARGET_FILE:ezc>"
                --yarpgen-bin "${YARPGEN_BIN}"
                --ref-compiler "${REFERENCE_C_COMPILER}"
                --target "${DEFAULT_TARGET_TRIPLE}"
                --seeds-file "${SEEDS_FILE}"
                --work-dir "${CMAKE_CURRENT_BINARY_DIR}/seeds_work"
                --reproducers-dir "${CMAKE_BINARY_DIR}/yarpgen_reproducers"
        )
        set_tests_properties("EzCompilerYarpgen_RegressionSeeds" PROPERTIES
            RUN_SERIAL TRUE
            TIMEOUT 240
            LABELS "Differential;Yarpgen;Regression;EzCompiler"
        )
    endif()
endif()
