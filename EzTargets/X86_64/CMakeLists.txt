add_subdirectory(Dsl)

add_subdirectory(Registration)

# x86-64 target subproject.
#
# Defines the runtime target library (EzTargetsX86_64) and drives the DSL code generators
# that synthesize its calling conventions, legalizer tables, instruction table, encoding
# table and instruction selector. Generated tables are emitted into the EzTargets::X86_64
# namespace and compiled into this same library.

set(EZTARGETS_X86_64_FILES_LIST
        "include/EzTargetsX86_64Common.h"
        "include/X86_64TargetDesc.h"
        "include/X86_64Lowering.h"
        "include/X86_64FrameLowerer.h"
        "include/X86_64RegisterAllocator.h"
        "include/X86_64TargetInstructionSelector.h"
        "include/X86_64ElfBinaryDesc.h"
        "include/X86_64CoffBinaryDesc.h"
        "include/X86_64RelocationResolver.h"
        "include/X86_64CodeEmitter.h"
        "include/Encoding/X86_64EncodingDesc.h"
        "include/Encoding/X86_64InstructionEncoder.h"
        "include/BranchRelaxation/BranchRelaxer.h"

        "src/X86_64TargetDesc.cpp"
        "src/X86_64Lowering.cpp"
        "src/X86_64FrameLowerer.cpp"
        "src/X86_64RegisterAllocator.cpp"
        "src/X86_64TargetInstructionSelector.cpp"
        "src/X86_64ElfBinaryDesc.cpp"
        "src/X86_64CoffBinaryDesc.cpp"
        "src/X86_64RelocationResolver.cpp"
        "src/X86_64CodeEmitter.cpp"
        "src/Encoding/X86_64InstructionEncoder.cpp"
        "src/BranchRelaxation/BranchRelaxer.cpp"
)

EzCMK_ConfigureLib(NAME "EzTargetsX86_64"
        FILE_LIST ${EZTARGETS_X86_64_FILES_LIST}
        PRECOMPILED_HEADER "include/EzTargetsX86_64Common.h"
        LINKED_LIBRARIES "EzTriple;EzCodeEmitter;EzMir;EzCore"
        INCLUDED_DIRS "include")

set(EZ_X86_64_NAMESPACE_ROOT "EzTargets::X86_64")

include("${CMAKE_CURRENT_SOURCE_DIR}/../CMake/EzDslGenCallingConv.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../CMake/EzDslGenLegalizerActionTable.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../CMake/EzDslGenTargetInstructions.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../CMake/EzDslGenInstructionSelector.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../CMake/EzDslGenRegisterInfo.cmake")

EzDslGenRegisterInfo(
    TARGET EzTargetsX86_64
    INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/targets/x86_64/x86_64.tdesc"
    TARGET_NAME "x86_64"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/x86_64"
)

EzDslGenCallingConv(
    TARGET EzTargetsX86_64
    INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/targets/x86_64/x86_64_calling_conv.ezcc"
    TARGET_NAME "x86_64"
    NAMESPACE_ROOT "${EZ_X86_64_NAMESPACE_ROOT}"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/x86_64"
)

EzDslGenLegalizerActionTable(
    TARGET EzTargetsX86_64
    INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/targets/x86_64/x86_64_legalize.lad"
    RULES  "${CMAKE_CURRENT_SOURCE_DIR}/targets/x86_64/x86_64_rules.lrd"
    TARGET_NAME "x86_64"
    NAMESPACE_ROOT "${EZ_X86_64_NAMESPACE_ROOT}"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/x86_64"
)

EzDslGenTargetInstructions(
    TARGET EzTargetsX86_64
    INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/targets/x86_64/x86_64_instructions.idf"
    TARGET_NAME "x86_64"
    NAMESPACE_ROOT "${EZ_X86_64_NAMESPACE_ROOT}"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/x86_64"
)

EzDslGenInstructionSelector(
    TARGET EzTargetsX86_64
    INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/targets/x86_64/x86_64_patterns.isf"
    TARGET_NAME "x86_64"
    NAMESPACE_ROOT "${EZ_X86_64_NAMESPACE_ROOT}"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/x86_64"
)
