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
            "include/parser/tokenizer/BasicTokenizer.h"
            "include/parser/tokenizer/InstructionTokenTable.h"
            "include/parser/tokenizer/ITokenizer.h"
            "include/parser/tokenizer/KeywordTokenMap.h"
            "include/parser/tokenizer/TypeTokenMap.h"

            "src/parser/tokenizer/BasicTokenizer.cpp"
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

    add_custom_target(CopyTestFiles ALL
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/testData"
            "$<TARGET_FILE_DIR:EzIRBuilder>/testData"
            COMMENT "Copying test data"
    )

    EzPacker_AddTest(
            "T_BasicTokenizer"
            "-DEZIRBUILDER"
            "EzIRBuilder"
            "CopyTestFiles"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/TestTokenizer.cpp"
    )
endif ()
