set(EZDSL_FILES_LIST
        "include/EzDslCommon.h"
        "include/Ast/CommonAstNodes.h"
        "include/Ast/InstructionDefLangAst.h"
        "include/Ast/InstructionSelDefLang.h"
        "include/Ast/LegalizeActionDefLangAst.h"
        "include/Ast/LegalizeRuleDefLangAst.h"
        "include/Ast/TargetDefLangAst.h"
        "include/Parser/CommonParsers.h"
        "include/Parser/InstructionDefLang.h"
        "include/Parser/LegalizeActionDefLang.h"
        "include/Parser/ParseContext.h"
        "include/Parser/TargetDefLang.h"

        "src/Parser/ParseContext.cpp"
)

EzCMK_ConfigureLib(NAME "EzDsl"
        FILE_LIST ${EZDSL_FILES_LIST}
        PRECOMPILED_HEADER "include/EzDslCommon.h"
        LINKED_LIBRARIES "EzCore;foonathan::lexy"
        INCLUDED_DIRS "include"
)

if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzDsl")

    EzPacker_AddTest(
            "T_TargetDefLang"
            "EzDsl"
            "GTest::gtest_main"
            "tests/T_TargetDefLang.cpp"
    )

    EzPacker_AddTest(
            "T_LegalizeActionDefLang"
            "EzDsl"
            "GTest::gtest_main"
            "tests/T_LegalizeActionDefLang.cpp"
    )

    EzPacker_AddTest(
            "T_LegalizeRuleDefLang"
            "EzDsl"
            "GTest::gtest_main"
            "tests/T_LegalizeRuleDefLang.cpp"
    )

    EzPacker_AddTest(
            "T_InstructionSelDefLang.cpp"
            "EzDsl"
            "GTest::gtest_main"
            "tests/T_InstructionSelDefLang.cpp"
    )
endif ()
