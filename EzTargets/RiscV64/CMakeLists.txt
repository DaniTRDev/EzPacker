add_subdirectory(Dsl)

# RISC-V 64 target subproject.
#
# Defines the runtime target library (EzTargetsRiscV64) and drives the DSL code generators
# that synthesize its calling conventions, legalizer tables, instruction table, encoding
# table and instruction selector. Generated tables are emitted into the EzTargets::RiscV64
# namespace and compiled into this same library.

set(EZTARGETS_RISCV64_FILES_LIST
        "include/EzTargetsRiscV64Common.h"
        "include/InstructionSelector/RiscV64TargetInstructionSelector.h"
        "include/FrameLowerer/RiscV64FrameLowerer.h"
        "include/RegisterAllocator/RiscV64RegisterAllocator.h"
        "include/Lowering/RiscV64Lowering.h"
        "include/Descriptors/RiscV64ElfBinaryDesc.h"
        "include/Relocation/RiscV64RelocationResolver.h"
        "include/CodeEmitter/RiscV64CodeEmitter.h"
        "include/Encoding/RiscV64EncodingDesc.h"
        "include/Encoding/RiscV64InstructionEncoder.h"
        "include/BranchRelaxation/RiscV64BranchRelaxer.h"

        "src/InstructionSelector/RiscV64TargetInstructionSelector.cpp"
        "src/FrameLowerer/RiscV64FrameLowerer.cpp"
        "src/RegisterAllocator/RiscV64RegisterAllocator.cpp"
        "src/Lowering/RiscV64Lowering.cpp"
        "src/Descriptors/RiscV64ElfBinaryDesc.cpp"
        "src/Relocation/RiscV64RelocationResolver.cpp"
        "src/CodeEmitter/RiscV64CodeEmitter.cpp"
        "src/Encoding/RiscV64InstructionEncoder.cpp"
        "src/BranchRelaxation/RiscV64BranchRelaxer.cpp"
)

EzCMK_ConfigureLib(NAME "EzTargetsRiscV64"
        FILE_LIST ${EZTARGETS_RISCV64_FILES_LIST}
        PRECOMPILED_HEADER "include/EzTargetsRiscV64Common.h"
        LINKED_LIBRARIES "EzTriple;EzCodeEmitter;EzMir;EzCore"
        INCLUDED_DIRS "include;include/InstructionSelector;include/FrameLowerer;include/RegisterAllocator;include/Lowering;include/Descriptors;include/Relocation;include/CodeEmitter;include/BranchRelaxation;include/Encoding")

set(EZ_RISCV64_NAMESPACE_ROOT "EzTargets::RiscV64")

include("${CMAKE_CURRENT_SOURCE_DIR}/../CMake/EzDslGenCallingConv.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../CMake/EzDslGenLegalizerActionTable.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../CMake/EzDslGenTargetInstructions.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../CMake/EzDslGenInstructionSelector.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../CMake/EzDslGenRegisterInfo.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../CMake/EzDslGenTargetDesc.cmake")

EzDslGenRegisterInfo(
    TARGET EzTargetsRiscV64
    INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/targets/riscv64/riscv64.tdesc"
    TARGET_NAME "RiscV64"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/riscv64"
)

EzDslGenTargetDesc(
    TARGET EzTargetsRiscV64
    INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/targets/riscv64/riscv64.tdesc"
    TARGET_NAME "RiscV64"
    NAMESPACE_ROOT "EzTargets"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/riscv64"
)

EzDslGenCallingConv(
    TARGET EzTargetsRiscV64
    INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/targets/riscv64/riscv64_calling_conv.ezcc"
    TARGET_NAME "riscv64"
    NAMESPACE_ROOT "${EZ_RISCV64_NAMESPACE_ROOT}"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/riscv64"
)

EzDslGenLegalizerActionTable(
    TARGET EzTargetsRiscV64
    INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/targets/riscv64/riscv64_legalize.lad"
    RULES  "${CMAKE_CURRENT_SOURCE_DIR}/targets/riscv64/riscv64_rules.lrd"
    TARGET_NAME "riscv64"
    NAMESPACE_ROOT "${EZ_RISCV64_NAMESPACE_ROOT}"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/riscv64"
)

EzDslGenTargetInstructions(
    TARGET EzTargetsRiscV64
    INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/targets/riscv64/riscv64_instructions.idf"
    TARGET_NAME "riscv64"
    NAMESPACE_ROOT "${EZ_RISCV64_NAMESPACE_ROOT}"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/riscv64"
)

EzDslGenInstructionSelector(
    TARGET EzTargetsRiscV64
    INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/targets/riscv64/riscv64_patterns.isf"
    TARGET_NAME "riscv64"
    NAMESPACE_ROOT "${EZ_RISCV64_NAMESPACE_ROOT}"
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/riscv64"
)

add_subdirectory(Registration)
