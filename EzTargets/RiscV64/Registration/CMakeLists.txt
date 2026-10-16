# RISC-V 64 target factory registration.
#
# Bridges the RISC-V 64 target into EzCompiler's TargetResolver.

set(EZTARGETS_RISCV64_REGISTRATION_FILES_LIST
        "include/EzTargetsRiscV64Registration.h"
        "RiscV64TargetRegistration.cpp"
)

EzCMK_ConfigureLib(NAME "EzTargetsRiscV64Registration"
        FILE_LIST ${EZTARGETS_RISCV64_REGISTRATION_FILES_LIST}
        PRECOMPILED_HEADER ""
        LINKED_LIBRARIES "EzTargetsRiscV64;EzCompilerLib"
        INCLUDED_DIRS "include;${CMAKE_SOURCE_DIR}/EzCompiler/include;${CMAKE_SOURCE_DIR}/EzTargets/RiscV64/include;${CMAKE_SOURCE_DIR}/EzMir/include;${CMAKE_SOURCE_DIR}/EzTriple/include;${CMAKE_SOURCE_DIR}/EzCore/include")
