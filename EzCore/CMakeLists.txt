# EzCore CMake build configuration

function(GetLibraryBuildType out)
    if (EZPACKER_BUILD_STATIC)
        set(${out} "STATIC" PARENT_SCOPE)
    else ()
        set(${out} "SHARED" PARENT_SCOPE)
    endif ()
endfunction()

function(BuildEzCore)
    GetLibraryBuildType(EZCORE_BUILD_TYPE)
    message(STATUS "Building EzCore as ${EZCORE_BUILD_TYPE}")

    add_library("EzCore" ${EZCORE_BUILD_TYPE}
            "include/EzCore.h"
            "include/EzCoreCommon.h"
            "include/ErrorCollector/ErrorCollector.h"
            "include/ErrorCollector/ErrorEmitter.h"
            "include/Logger/SourceLoggingSink.h"
            "include/SourceManager/SourceManager.h"
            "include/TypedPool/TypedArrayPool.h"
            "include/TypedPool/TypedPool.h"

            "src/ErrorCollector/ErrorCollector.cpp"
            "src/ErrorCollector/ErrorEmitter.cpp"
            "src/Logger/SourceLogger.cpp"
            "src/SourceManager/SourceManager.cpp"
    )

    # Set precompiled header
    target_precompile_headers(EzCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/EzCoreCommon.h")

    # Add include directories and link dependencies
    target_link_libraries(EzCore PUBLIC EzLogger gtest gtest_main)
    target_include_directories(EzCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" ${gtest_build_include_dirs})
endfunction()

BuildEzCore()

# Add tests
if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzCore")

    #EzPacker_AddTest(
    #       "T_EzCoreTest"
    #       ""
    #       "EzCore"
    #       ""
    #       "${CMAKE_CURRENT_SOURCE_DIR}/tests/TestEzCore.cpp"
    #)
endif ()
