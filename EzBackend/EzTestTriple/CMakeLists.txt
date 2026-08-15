# This project defines a TEST triple (target+abi+calling conv) that is used for the rest of the tests.

set(EZTESTTRIPLE_FILES_LIST
        "include/EzTestTriple.h"
        "include/EzTestTripleCommon.h"
        "include/EzTestTripleTargetDesc.h"
        "include/BinaryDescriptors/EzTestTripleBinaryDesc.h"
        "include/CallingConvs/EzTestTripleCallingConv.h"
        "include/FrameLowerer/EzTestTripleFrameLowerer.h"
        "include/InstructionSelector/EzTestTripleInstructionSelector.h"
        "include/InstructionSelector/EzTestTripleInstructionSet.h"
        "include/InstructionSelector/EzTestTripleInstructionSetDefs.h"
        "include/Legalizer/EzTestTripleExpansionRegistry.h"
        "include/Legalizer/EzTestTripleLegalizer.h"
        "include/RegisterAllocator/EzTestTripleRegisterAllocator.h"
        "include/RegisterBanks/EzTestTripleRegisterBanks.h"
        "include/Type/EzTestTripleTypeLayout.h"

        "src/EzTestTripleTargetDesc.cpp"
        "src/BinaryDescriptors/EzTestTripleBinaryDesc.cpp"
        "src/CallingConvs/EzTestTripleCallingConv.cpp"
        "src/FrameLowerer/EzTestTripleFrameLowerer.cpp"
        "src/InstructionSelector/EzTestTripleInstructionSelector.cpp"
        "src/InstructionSelector/EzTestTripleInstructionSet.cpp"
        "src/Legalizer/EzTestTripleExpansionRegistry.cpp"
        "src/Legalizer/EzTestTripleLegalizer.cpp"
        "src/RegisterAllocator/EzTestTripleRegisterAllocator.cpp"
        "src/RegisterBanks/EzTestTripleRegisterBanks.cpp"
        "src/Type/EzTestTripleTypeLayout.cpp"
)

EzCMK_ConfigureLib(NAME "EzTestTriple"
        FILE_LIST ${EZTESTTRIPLE_FILES_LIST}
        PRECOMPILED_HEADER "include/EzTestTripleCommon.h"
        LINKED_LIBRARIES "EzTriple"
        INCLUDED_DIRS "include")
