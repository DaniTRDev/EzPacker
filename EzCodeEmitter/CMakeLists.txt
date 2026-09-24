set(EZCODEEMITTER_FILES_LIST
        "include/CodeEmitterContext.h"
        "include/CodeSection.h"
        "include/EzCodeEmitter.h"
        "include/EzCodeEmitterCommon.h"
        "include/GenericCodeEmitter.h"
        "include/Helpers.h"
        "include/Encoding/EncodeResult.h"
        "include/ObjectFormat/ObjectSymbol.h"
        "include/ObjectFormat/IObjectWriter.h"
        "include/ObjectFormat/Elf64Writer.h"
        "include/ObjectFormat/CoffWriter.h"

        "src/CodeEmitterContext.cpp"
        "src/CodeSection.cpp"
        "src/Helpers.cpp"
        "src/ObjectFormat/Elf64Writer.cpp"
        "src/ObjectFormat/CoffWriter.cpp"
)

EzCMK_ConfigureLib(NAME "EzCodeEmitter"
        FILE_LIST ${EZCODEEMITTER_FILES_LIST}
        PRECOMPILED_HEADER "include/EzCodeEmitterCommon.h"
        LINKED_LIBRARIES "EzMir"
        INCLUDED_DIRS "include")
