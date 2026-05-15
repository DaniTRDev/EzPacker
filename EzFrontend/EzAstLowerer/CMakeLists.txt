set(EZASTLOWERER_FILES_LIST
        "include/AstLowererVisitor.h"
        "include/EzAstLowerer.h"
        "include/EzAstLowererCommon.h"
        "include/GenericAstLowerer.h"
        "include/AstLoweringContext.h"
        "include/Lowerers/BreakLowerer.h"
        "include/Lowerers/CodeScopeLowerer.h"
        "include/Lowerers/ConditionLowerer.h"
        "include/Lowerers/ContinueLowerer.h"
        "include/Lowerers/ForLowerer.h"
        "include/Lowerers/IfLowerer.h"
        "include/Lowerers/ImmediateLowerer.h"
        "include/Lowerers/InstructionLowerer.h"
        "include/Lowerers/LabelLowerer.h"
        "include/Lowerers/ModuleLowerer.h"
        "include/Lowerers/SwitchLowerer.h"
        "include/Lowerers/TypeLowerer.h"
        "include/Lowerers/VariableLowerer.h"
        "include/Lowerers/WhileLowerer.h"

        "src/AstLowererVisitor.cpp"
        "src/AstLoweringContext.cpp"
        "src/Lowerers/BreakLowerer.cpp"
        "src/Lowerers/CodeScopeLowerer.cpp"
        "src/Lowerers/ConditionLowerer.cpp"
        "src/Lowerers/ContinueLowerer.cpp"
        "src/Lowerers/ForLowerer.cpp"
        "src/Lowerers/IfLowerer.cpp"
        "src/Lowerers/ImmediateLowerer.cpp"
        "src/Lowerers/InstructionLowerer.cpp"
        "src/Lowerers/LabelLowerer.cpp"
        "src/Lowerers/ModuleLowerer.cpp"
        "src/Lowerers/SwitchLowerer.cpp"
        "src/Lowerers/TypeLowerer.cpp"
        "src/Lowerers/VariableLowerer.cpp"
        "src/Lowerers/WhileLowerer.cpp"
)

EzCMK_ConfigureLib(NAME "EzAstLowerer"
        FILE_LIST ${EZASTLOWERER_FILES_LIST}
        PRECOMPILED_HEADER "include/EzAstLowererCommon.h"
        LINKED_LIBRARIES "EzSemantics;EzMir"
        INCLUDED_DIRS "include")

if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzAstLowerer")

    EzPacker_AddTest(
            "T_AstLowerer"
            "EzAstLowerer"
            ""
            "tests/T_AstLowerer.cpp"
    )
endif ()