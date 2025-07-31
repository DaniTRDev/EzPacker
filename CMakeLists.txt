cmake_minimum_required(VERSION 3.27)
project(EzPacker)

set(CMAKE_CXX_STANDARD 23)
set(EZPACKER_COMMON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Common") # Path for Common files used across projects.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

option(EZPACKER_BUILD_TESTS "Builds tests for EzPacker" ON)

set(EZPACKER_BUILD_STATIC OFF CACHE BOOL "Builds EzPacker as STATIC" FORCE)

if (EZPACKER_BUILD_TESTS)
    message(STATUS "Enabling CTest on EzPacker")
    enable_testing()

    include("CMake/Tests.cmake")
endif ()

include("CMake/Vendor.cmake") # Fetch 3rd party libs.

add_subdirectory(EzCore)
add_subdirectory(EzFrontend)

if (EZPACKER_BUILD_TESTS)
    EzPacker_BuildTests("${CMAKE_CURRENT_SOURCE_DIR}/CMake/TestMain.cpp")
endif ()
