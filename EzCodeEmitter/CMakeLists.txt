set(EZCODEEMITTER_FILES_LIST
        "include/CodeEmitterContext.h"
        "include/CodeSection.h"
        "include/EzCodeEmitter.h"
        "include/EzCodeEmitterCommon.h"
        "include/GenericCodeEmitter.h"
        "include/Helpers.h"
        "include/X86_64/X86_64Registers.h"
        "include/X86_64/X86_64Encoding.h"
        "include/X86_64/X86_64CodeEmitter.h"
        "include/TableGen/EncodingDesc.h"
        "include/TableGen/InstructionEncoder.h"
        "include/BranchRelaxation/BranchRelaxer.h"
        "include/ObjectFormat/ObjectSymbol.h"
        "include/ObjectFormat/Elf64Writer.h"
        "include/ObjectFormat/CoffWriter.h"

        "src/CodeEmitterContext.cpp"
        "src/CodeSection.cpp"
        "src/Helpers.cpp"
        "src/X86_64/X86_64Encoding.cpp"
        "src/X86_64/X86_64CodeEmitter.cpp"
        "src/TableGen/InstructionEncoder.cpp"
        "src/BranchRelaxation/BranchRelaxer.cpp"
        "src/ObjectFormat/Elf64Writer.cpp"
        "src/ObjectFormat/CoffWriter.cpp"
)

EzCMK_ConfigureLib(NAME "EzCodeEmitter"
        FILE_LIST ${EZCODEEMITTER_FILES_LIST}
        PRECOMPILED_HEADER "include/EzCodeEmitterCommon.h"
        LINKED_LIBRARIES "EzMir"
        INCLUDED_DIRS "include")
