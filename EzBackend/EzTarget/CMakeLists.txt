set(EZTARGET_FILES_LIST
        "include/EzTarget.h"
        "include/EzTargetCommon.h"
        "include/TargetDesc.h"
        "include/TargetAbiLowerer/TargetAbiLowererContext.h"
        "include/TargetAbiLowerer/TargetAbiLowererPass.h"
        "include/TargetInstructionSelector/InstructionSelectorContext.h"
        "include/TargetInstructionSelector/InstructionSelectorPass.h"
        "include/TargetLegalizer/AbiLegalizerPass.h"
        "include/TargetLegalizer/LegalizerActionList.h"
        "include/TargetLegalizer/LegalizerContext.h"
        "include/TargetLegalizer/LegalizerHandlerList.h"
        "include/TargetLegalizer/TypeLegalizerPass.h"
        "include/TargetLegalizer/StandardLegalizers/ExpandTypeLegalizer.h"
        "include/TargetLegalizer/StandardLegalizers/PromoteTypeLegalizer.h"

        "src/TargetDesc.cpp"
        "src/TargetAbiLowerer/TargetAbiLowererContext.cpp"
        "src/TargetAbiLowerer/TargetAbiLowererPass.cpp"
        "src/TargetInstructionSelector/InstructionSelectorPass.cpp"
        "src/TargetLegalizer/LegalizerActionList.cpp"
        "src/TargetLegalizer/LegalizerContext.cpp"
        "src/TargetLegalizer/LegalizerHandlerList.cpp"
        "src/TargetLegalizer/TypeLegalizerPass.cpp"
        "src/TargetLegalizer/StandardLegalizers/ExpandTypeLegalizer.cpp"
        "src/TargetLegalizer/StandardLegalizers/PromoteTypeLegalizer.cpp"
)

EzCMK_ConfigureLib(NAME "EzTarget"
        FILE_LIST ${EZTARGET_FILES_LIST}
        PRECOMPILED_HEADER "include/EzTargetCommon.h"
        LINKED_LIBRARIES "EzMir;EzABI"
        INCLUDED_DIRS "include")

if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzTarget")

    EzPacker_AddTest(
            "T_PromoteTypeLegalizer"
            "EzTarget"
            ""
            "tests/T_PromoteTypeLegalizer.cpp"
    )

    EzPacker_AddTest(
            "T_ExpandTypeLegalizer"
            "EzTarget"
            ""
            "tests/T_ExpandTypeLegalizer.cpp"
    )

    EzPacker_AddTest(
            "T_TypeLegalizerPass"
            "EzTarget"
            ""
            "tests/T_TypeLegalizerPass.cpp"
    )

    EzPacker_AddTest(
            "T_AArch64Flow"
            "EzTarget"
            ""
            "tests/T_AArch64Flow.cpp"
    )

    EzPacker_AddTest(
            "T_TargetAbiLowerer"
            "EzTarget"
            ""
            "tests/T_TargetAbiLowerer.cpp"
    )

endif ()
