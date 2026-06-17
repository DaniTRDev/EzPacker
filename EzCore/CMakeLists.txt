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
            "include/StringUtils.h"
            "include/Diagnostics/DiagnosticBuilder.h"
            "include/Diagnostics/DiagnosticCollector.h"
            "include/Diagnostics/DiagnosticListener.h"
            "include/Diagnostics/DiagnosticLogger.h"
            "include/Diagnostics/DiagnosticMessage.h"
            "include/Diagnostics/DiagnosticScope.h"
            "include/SourceManager/SourceManager.h"

            "src/Diagnostics/DiagnosticBuilder.cpp"
            "src/Diagnostics/DiagnosticCollector.cpp"
            "src/Diagnostics/DiagnosticLogger.cpp"
            "src/Diagnostics/DiagnosticMessage.cpp"
            "src/Diagnostics/DiagnosticScope.cpp"
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

    EzPacker_AddTest(
            "T_DiagnosticLogger"
            "EzCore"
            ""
            "tests/T_DiagnosticLogger.cpp"
    )
endif ()
