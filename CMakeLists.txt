cmake_minimum_required(VERSION 3.27)
project(EzPacker)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

option(EZPACKER_BUILD_TESTS "Builds tests for EzPacker" ON)
set(EZPACKER_BUILD_STATIC OFF CACHE BOOL "Builds EzPacker as STATIC" FORCE)

include(FetchContent)
message(STATUS "Retrieving EzLib")
enable_testing()

set(EZLIB_BUILD_TESTS OFF CACHE BOOL "Builds tests for EzLib" FORCE)
set(EZLIB_BUILD_STATIC OFF CACHE BOOL "Build EzLib as a SHARED lib" FORCE)
FetchContent_Declare(EzLib
        GIT_REPOSITORY https://github.com/DaniTRDev/EzLib
        GIT_TAG 704f4428570f54fb74f4234b0f659f728bd5bb36
)
FetchContent_MakeAvailable(EzLib)
EzCMK_Bootstrap()
EzCMK_SetLibraryBuildType("SHARED")

include("CMake/Vendor.cmake") # Fetch 3rd party libs.
include("CMake/TestWrapper.cmake")

if (EZPACKER_BUILD_TESTS)
    EzCMK_EnableTestsForCurrentProject()
endif ()

add_subdirectory(LibBf) # https://bellard.org/libbf/
add_subdirectory(EzCore)
add_subdirectory(EzMir)
add_subdirectory(EzDsl)
add_subdirectory(EzTriple)
add_subdirectory(EzCodeEmitter)

if (EZPACKER_BUILD_TESTS)
    message(STATUS "Configuring EzMirTestSuite")
    add_subdirectory(tests/EzMirTestSuite)

    message(STATUS "Configuring EzDslTestSuite")
    add_subdirectory(tests/EzDslTestSuite)

    message(STATUS "Configuring EzTripleTestSuite")
    add_subdirectory(tests/EzTripleTestSuite)

    EzPacker_BuildTests("${CMAKE_CURRENT_SOURCE_DIR}/CMake/TestMain.cpp")
endif ()