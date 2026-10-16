cmake_minimum_required(VERSION 3.27)

option(EZPACKER_BUILD_YARPGEN_TESTS "Enable YARPGen-based differential tests" ON)
if(NOT EZPACKER_BUILD_YARPGEN_TESTS)
    return()
endif()

message(STATUS "Configuring EzPacker YARPGen Differential Test Suite...")

# 1. Locate Python 3 Interpreter
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# 2. Locate or Fetch Intel YARPGen
set(YARPGEN_EXECUTABLE "" CACHE FILEPATH "Path to pre-built yarpgen executable")
set(YARPGEN_CANDIDATE "$ENV{YARPGEN_BIN}")

if(NOT YARPGEN_EXECUTABLE AND YARPGEN_CANDIDATE AND EXISTS "${YARPGEN_CANDIDATE}")
    set(YARPGEN_EXECUTABLE "${YARPGEN_CANDIDATE}")
endif()

if(NOT YARPGEN_EXECUTABLE)
    # Check scratch location if present
    set(SCRATCH_YARPGEN "${CMAKE_CURRENT_SOURCE_DIR}/../../scratch/yarpgen/build/yarpgen${CMAKE_EXECUTABLE_SUFFIX}")
    if(EXISTS "${SCRATCH_YARPGEN}")
        set(YARPGEN_EXECUTABLE "${SCRATCH_YARPGEN}")
    endif()
endif()

if(NOT YARPGEN_EXECUTABLE OR NOT EXISTS "${YARPGEN_EXECUTABLE}")
    message(STATUS "[YarpgenDifferentialTests] Fetching Intel YARPGen from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
        yarpgen_ext
        GIT_REPOSITORY https://github.com/intel/yarpgen.git
        GIT_TAG 1adb290f453505838f3aa33dc571f292b0c1810f
    )
    FetchContent_MakeAvailable(yarpgen_ext)
    set(YARPGEN_BIN "$<TARGET_FILE:yarpgen>")
    set(YARPGEN_DEP yarpgen)
else()
    message(STATUS "[YarpgenDifferentialTests] Using existing YARPGen: ${YARPGEN_EXECUTABLE}")
    set(YARPGEN_BIN "${YARPGEN_EXECUTABLE}")
    set(YARPGEN_DEP "")
endif()

# 3. Reference Compiler (GCC or Clang)
if(CMAKE_C_COMPILER)
    set(REFERENCE_C_COMPILER "${CMAKE_C_COMPILER}")
else()
    find_program(REFERENCE_C_COMPILER NAMES gcc clang)
endif()

if(NOT REFERENCE_C_COMPILER)
    message(WARNING "[YarpgenDifferentialTests] No C compiler (GCC/Clang) found for reference testing. Differential tests disabled.")
    return()
endif()

message(STATUS "[YarpgenDifferentialTests] Reference compiler: ${REFERENCE_C_COMPILER}")

set(YARPGEN_RUNNER "${CMAKE_CURRENT_SOURCE_DIR}/yarpgen_runner.py")
set(SEEDS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/regression_seeds.txt")

# Target triple setting
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(DEFAULT_TARGET_TRIPLE "x86_64-pc-windows-coff")
else()
    set(DEFAULT_TARGET_TRIPLE "x86_64-unknown-linux-gnu")
endif()

# 4. Custom target for launching on-demand differential fuzzing campaigns
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

# 5. Deterministic Regression Tests for CTest
if(EXISTS "${SEEDS_FILE}")
    file(STRINGS "${SEEDS_FILE}" REGRESSION_SEEDS)
    foreach(LINE IN LISTS REGRESSION_SEEDS)
        string(STRIP "${LINE}" LINE)
        if(LINE MATCHES "^#" OR LINE STREQUAL "")
            continue()
        endif()

        # Extract seed (first whitespace-delimited word)
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
                LABELS "Differential;Yarpgen;EzCompiler"
            )
        endif()
    endforeach()
endif()
