set(EZTRIPLE_FILES_LIST
        "include/EzTripleCommon.h"
        "include/AbiLowerer/MirAbiLowerer.h"
        "include/AbiLowerer/MirAbiLowererPass.h"
        "include/FrameLowerer/MirFrameLowerer.h"
        "include/FrameLowerer/MirFrameLowererPass.h"
        "include/Descriptors/TargetDesc.h"
        "include/Descriptors/TargetBinaryDesc.h"
        "include/RegisterAllocator/MirRegisterAllocator.h"
        "include/RegisterAllocator/MirRegisterAllocatorPass.h"

        "src/AbiLowerer/MirAbiLowerer.cpp"
        "src/AbiLowerer/MirAbiLowererPass.cpp"
        "src/FrameLowerer/MirFrameLowerer.cpp"
        "src/FrameLowerer/MirFrameLowererPass.cpp"
        "src/RegisterAllocator/MirRegisterAllocator.cpp"
        "src/RegisterAllocator/MirRegisterAllocatorPass.cpp"
)

EzCMK_ConfigureLib(NAME "EzTriple"
        FILE_LIST ${EZTRIPLE_FILES_LIST}
        PRECOMPILED_HEADER "include/EzTripleCommon.h"
        LINKED_LIBRARIES "EzMir"
        INCLUDED_DIRS "include")
