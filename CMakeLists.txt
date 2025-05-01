cmake_minimum_required(VERSION 3.29)
project(EzPacker)

set(CMAKE_CXX_STANDARD 23)
set(EZPACKER_COMMON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Common") # Path for Common files used across projects.
option(EZPACKER_BUILD_TESTS "Builds tests for EzPacker" ON)

set(EZPACKER_BUILD_STATIC ON CACHE BOOL "Builds EzPacker as STATIC" FORCE)
#option(EZPACKER_BUILD_STATIC "Build libraries as STATIC" ON)

if (EZPACKER_BUILD_TESTS)
    message(STATUS "Enabling CTest on EzPacker")
    enable_testing()
    include("CMake/Tests.cmake")
endif ()

include("CMake/Vendor.cmake") # Fetch 3rd party libs.

add_subdirectory(EzLifter) # Add lifter library.

