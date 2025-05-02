function(GetLibraryBuildType out)
    if (EZPACKER_BUILD_STATIC)
        set(${out} "STATIC" PARENT_SCOPE)
    else ()
        set(${out} "SHARED" PARENT_SCOPE)
    endif ()
endfunction()

function(BuildIRBuilder)
    GetLibraryBuildType(EZIRBUILDER_BUILD_TYPE)
    message(STATUS "Building EzIRBuilder as ${EZIRBUILDER_BUILD_TYPE}")

    add_library("EzIRBuilder" ${EZIRBUILDER_BUILD_TYPE}
            "include/EzIRBuilder.h"
            "include/EzIRBuilderCommon.h"
    )

    # Set precompiled header.
    target_precompile_headers(EzIRBuilder PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/EzIRBuilderCommon.h")

    # Adds EzIRBuilder's own directory to include path and link GTest, Zydis and LLVM.
    target_link_libraries(EzIRBuilder PUBLIC EzLogger gtest gtest_main)
    target_include_directories(EzIRBuilder PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" ${gtest_build_include_dirs})
endfunction()

BuildIRBuilder()

# Add tests
if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzIRBuilder")
    #EzPacker_AddTest(
            #"T_EzZydisDecodedOperand"
            #"-DEZLIFTER"
            #"EzLifter"
            #"${CMAKE_CURRENT_SOURCE_DIR}/tests/TestEzZydisDecodedOperand.cpp"
    #)
endif ()
