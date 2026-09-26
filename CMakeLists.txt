cmake_minimum_required(VERSION 3.27)
project(EzPacker)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

option(EZPACKER_BUILD_TESTS "Builds tests for EzPacker" ON)
set(EZPACKER_BUILD_STATIC OFF CACHE BOOL "Builds EzPacker as STATIC" FORCE)

include(FetchContent)
message(STATUS "Retrieving EzLib")

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
include("CMake/CopyStandardLibraries.cmake")

option(EZPACKER_ENABLE_SANITIZERS "Enable Address and Undefined Behavior Sanitizers" OFF)
if (EZPACKER_ENABLE_SANITIZERS)
    if (MSVC)
        add_compile_options(/fsanitize=address)
    else ()
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    endif ()
endif ()

if (EZPACKER_BUILD_TESTS)
    enable_testing()
    EzCMK_EnableTestsForCurrentProject()
endif ()

add_subdirectory(LibBf) # https://bellard.org/libbf/
add_subdirectory(EzCore)
add_subdirectory(EzMir)
add_subdirectory(EzDsl)
add_subdirectory(EzCodeEmitter)
add_subdirectory(EzTriple)
add_subdirectory(EzCompiler)
add_subdirectory(EzTargets)

find_package(Doxygen OPTIONAL_COMPONENTS dot)
if (DOXYGEN_FOUND)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
    if (NOT TARGET doxygen)
        add_custom_target(doxygen DEPENDS docs)
    endif ()
    message(STATUS "Doxygen found (${DOXYGEN_VERSION}): 'docs' and 'doxygen' targets are available")
else ()
    message(STATUS "Doxygen not found: 'docs' and 'doxygen' targets are disabled")
endif ()


if (EZPACKER_BUILD_TESTS)
    add_subdirectory(tests/EzMirTestSuite)
    add_subdirectory(tests/EzDslLexerTestSuite)
    add_subdirectory(tests/EzDslSemaTestSuite)
    add_subdirectory(tests/EzDslCodeGeneratorsTestSuite)
    add_subdirectory(tests/EzDslCliTestSuite)
    add_subdirectory(tests/EzTripleTestSuite)
    add_subdirectory(tests/EzCodeEmitterTestSuite)
    add_subdirectory(tests/EzCompilerTestSuite)
    add_subdirectory(tests/EzCompilerEndToEndTests)

    EzPacker_BuildTests("${CMAKE_CURRENT_SOURCE_DIR}/CMake/TestMain.cpp")
endif ()

if (TARGET EzPacker_CopyStandardLibraries)
    if (TARGET ezc)
        add_dependencies(ezc EzPacker_CopyStandardLibraries)
    endif ()
    if (TARGET EzDslCli)
        add_dependencies(EzDslCli EzPacker_CopyStandardLibraries)
    endif ()
endif ()