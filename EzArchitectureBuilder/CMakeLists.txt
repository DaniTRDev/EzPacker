# EzArchitectureBuilder CMake build configuration

function(GetLibraryBuildType out)
    if (EZPACKER_BUILD_STATIC)
        set(${out} "STATIC" PARENT_SCOPE)
    else ()
        set(${out} "SHARED" PARENT_SCOPE)
    endif ()
endfunction()

function(BuildEzArchitectureBuilder)
    GetLibraryBuildType(EZARCHITECTUREBUILDER_BUILD_TYPE)
    message(STATUS "Building EzArchitectureBuilder as ${EZARCHITECTUREBUILDER_BUILD_TYPE}")

    add_library("EzArchitectureBuilder" ${EZARCHITECTUREBUILDER_BUILD_TYPE}
            "include/Architecture.h"
            "include/EzArchitectureBuilder.h"
            "include/EzArchitectureBuilderCommon.h"
            "include/semantics/SemanticRule.h"

            "src/Architecture.cpp"
            "src/EzArchitectureBuilder.cpp"
            "src/semantics/SemanticRule.cpp"
    )

    # Set precompiled header
    target_precompile_headers(EzArchitectureBuilder PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/EzArchitectureBuilderCommon.h")

    # Add include directories and link dependencies
    target_link_libraries(EzArchitectureBuilder PUBLIC EzLogger gtest gtest_main)
    target_include_directories(EzArchitectureBuilder PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" ${gtest_build_include_dirs})
endfunction()

BuildEzArchitectureBuilder()

# Add tests
if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzArchitectureBuilder")
endif()
