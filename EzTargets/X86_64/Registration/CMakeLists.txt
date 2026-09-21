# x86-64 target factory registration.
#
# Bridges the x86-64 target into EzCompiler's TargetResolver. It lives in its own library
# that depends on EzCompilerLib so the compiler core stays target-agnostic: only the
# executable and tests that need the target link this library.

set(EZTARGETS_X86_64_REGISTRATION_FILES_LIST
        "include/EzTargetsX86_64Registration.h"
        "X86_64TargetRegistration.cpp"
)

EzCMK_ConfigureLib(NAME "EzTargetsX86_64Registration"
        FILE_LIST ${EZTARGETS_X86_64_REGISTRATION_FILES_LIST}
        PRECOMPILED_HEADER ""
        LINKED_LIBRARIES "EzTargetsX86_64;EzCompilerLib"
        INCLUDED_DIRS "include;${CMAKE_SOURCE_DIR}/EzCompiler/include;${CMAKE_SOURCE_DIR}/EzTargets/X86_64/include;${CMAKE_SOURCE_DIR}/EzMir/include;${CMAKE_SOURCE_DIR}/EzTriple/include;${CMAKE_SOURCE_DIR}/EzCore/include")
