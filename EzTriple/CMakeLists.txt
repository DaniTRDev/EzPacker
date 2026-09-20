set(EZTRIPLE_FILES_LIST
        "include/EzTripleCommon.h"
        "include/AbiLowerer/MirAbiLowerer.h"
        "include/AbiLowerer/MirAbiLowererPass.h"
        "include/FrameLowerer/MirFrameLowerer.h"
        "include/FrameLowerer/MirFrameLowererPass.h"
        "include/Descriptors/TargetDesc.h"
        "include/Descriptors/BasicBinaryDesc.h"
        "include/Descriptors/TargetBinaryDesc.h"
        "include/Descriptors/TargetRelocationResolver.h"
        "include/InstructionSelector/MirAddressingModeMatcher.h"
        "include/InstructionSelector/MirInstructionSelector.h"
        "include/InstructionSelector/MirInstructionSelectorPass.h"
        "include/Legalizer/MirLegalizer.h"
        "include/Legalizer/MirLegalizerPass.h"
        "include/Legalizer/MirFunctionSignatureLegalizerPass.h"
        "include/Legalizer/Actions/LegalizeActionCommon.h"
        "include/Legalizer/Actions/LegalizeCallAction.h"
        "include/Legalizer/Actions/LegalizeReturnAction.h"
        "include/Legalizer/Actions/LegalizeWidenScalarAction.h"
        "include/Legalizer/Actions/LegalizeNarrowScalarAction.h"
        "include/Legalizer/Actions/LegalizeBitcastAction.h"
        "include/Legalizer/Actions/LegalizeLibcallAction.h"
        "include/RegisterAllocator/MirRegisterAllocator.h"
        "include/Targets/X86_64/X86_64FrameLowerer.h"
        "include/Targets/X86_64/X86_64Lowering.h"
        "include/Targets/X86_64/X86_64RegisterAllocator.h"
        "include/Targets/X86_64/X86_64TargetDesc.h"
        "include/Targets/X86_64/X86_64InstructionSelector.h"
        "include/Targets/X86_64/X86_64ElfBinaryDesc.h"
        "include/Targets/X86_64/X86_64CoffBinaryDesc.h"
        "include/Targets/X86_64/X86_64RelocationResolver.h"

        "src/AbiLowerer/MirAbiLowerer.cpp"
        "src/AbiLowerer/MirAbiLowererPass.cpp"
        "src/Descriptors/BasicBinaryDesc.cpp"
        "src/Descriptors/TargetDesc.cpp"
        "src/FrameLowerer/MirFrameLowerer.cpp"
        "src/FrameLowerer/MirFrameLowererPass.cpp"
        "src/InstructionSelector/MirAddressingModeMatcher.cpp"
        "src/InstructionSelector/MirInstructionSelector.cpp"
        "src/InstructionSelector/MirInstructionSelectorPass.cpp"
        "src/Legalizer/MirLegalizer.cpp"
        "src/Legalizer/MirLegalizerPass.cpp"
        "src/Legalizer/MirFunctionSignatureLegalizerPass.cpp"
        "src/Legalizer/Actions/LegalizeCallAction.cpp"
        "src/Legalizer/Actions/LegalizeReturnAction.cpp"
        "src/Legalizer/Actions/LegalizeWidenScalarAction.cpp"
        "src/Legalizer/Actions/LegalizeNarrowScalarAction.cpp"
        "src/Legalizer/Actions/LegalizeBitcastAction.cpp"
        "src/Legalizer/Actions/LegalizeLibcallAction.cpp"
        "src/RegisterAllocator/MirRegisterAllocator.cpp"
        "src/RegisterAllocator/MirRegisterAllocatorPass.cpp"
        "src/Targets/X86_64/X86_64Lowering.cpp"
        "src/Targets/X86_64/X86_64InstructionSelector.cpp"
        "src/Targets/X86_64/X86_64FrameLowerer.cpp"
        "src/Targets/X86_64/X86_64RegisterAllocator.cpp"
        "src/Targets/X86_64/X86_64TargetDesc.cpp"
        "src/Targets/X86_64/X86_64ElfBinaryDesc.cpp"
        "src/Targets/X86_64/X86_64CoffBinaryDesc.cpp"
        "src/Targets/X86_64/X86_64RelocationResolver.cpp"
)

EzCMK_ConfigureLib(NAME "EzTriple"
        FILE_LIST ${EZTRIPLE_FILES_LIST}
        PRECOMPILED_HEADER "include/EzTripleCommon.h"
        LINKED_LIBRARIES "EzMir;EzCodeEmitter"
        INCLUDED_DIRS "include;../EzCodeEmitter/include")

include(CMake/EzDslGenCallingConv.cmake)
include(CMake/EzDslGenLegalizerActionTable.cmake)
include(CMake/EzDslGenTargetInstructions.cmake)
include(CMake/EzDslGenInstructionSelector.cmake)

EzDslGenCallingConv(
    TARGET EzTriple
    INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/targets/x86_64/x86_64_calling_conv.ezcc"
    TARGET_NAME "x86_64"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/x86_64"
)

EzDslGenLegalizerActionTable(
    TARGET EzTriple
    INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/targets/x86_64/x86_64_legalize.lad"
    RULES  "${CMAKE_CURRENT_SOURCE_DIR}/targets/x86_64/x86_64_rules.lrd"
    TARGET_NAME "x86_64"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/x86_64"
)

EzDslGenTargetInstructions(
    TARGET EzTriple
    INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/targets/x86_64/x86_64_instructions.idf"
    TARGET_NAME "x86_64"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/x86_64"
)

EzDslGenInstructionSelector(
    TARGET EzTriple
    INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/targets/x86_64/x86_64_patterns.isf"
    TARGET_NAME "x86_64"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/x86_64"
)
