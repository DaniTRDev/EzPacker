set(EZDSL_FILES_LIST
        "include/EzDslCommon.h"
        "include/Ast/CommonAstNodes.h"
        "include/Ast/InstDefLangAst.h"
        "include/Ast/LegalizeMatrixDefLang.h"
        "include/Ast/LegalizeRuleDefLang.h"
        "include/Ast/TargetDefLangAst.h"
        "include/Parser/CommonParsers.h"
        "include/Parser/InstDefLang.h"
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
            "T_InstDefLang"
            "EzDsl"
            "GTest::gtest_main"
            "tests/T_InstDefLang.cpp"
    )
endif ()
