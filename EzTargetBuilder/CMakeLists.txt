# EzTargetBuilder CMake build configuration

function(GetLibraryBuildType out)
    if (EZPACKER_BUILD_STATIC)
        set(${out} "STATIC" PARENT_SCOPE)
    else ()
        set(${out} "SHARED" PARENT_SCOPE)
    endif ()
endfunction()

function(BuildEzTargetBuilder)
    GetLibraryBuildType(EZTARGETBUILDER_BUILD_TYPE)
    message(STATUS "Building EzTargetBuilder as ${EZTARGETBUILDER_BUILD_TYPE}")

    add_library("EzTargetBuilder" ${EZTARGETBUILDER_BUILD_TYPE}
            "include/EzTargetBuilder.h"
            "include/EzTargetBuilderCommon.h"
    )

    # Set precompiled header
    target_precompile_headers(EzTargetBuilder PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/EzTargetBuilderCommon.h")

    # Add include directories and link dependencies
    target_link_libraries(EzTargetBuilder PUBLIC EzLogger gtest gtest_main)
    target_include_directories(EzTargetBuilder PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" ${gtest_build_include_dirs})
endfunction()

BuildEzTargetBuilder()

# Add tests
if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzTargetBuilder")

    #EzPacker_AddTest(
    #       "T_EzTargetBuilderTest"
    #       ""
    #       "EzTargetBuilder"
    #       ""
    #       "${CMAKE_CURRENT_SOURCE_DIR}/tests/TestEzTargetBuilder.cpp"
    #)
endif()
