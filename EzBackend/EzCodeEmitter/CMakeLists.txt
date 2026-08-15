set(EZCODEEMITTER_FILES_LIST
        "include/CodeEmitterContext.h"
        "include/EzCodeEmitter.h"
        "include/EzCodeEmitterCommon.h"
        "include/GenericCodeEmitter.h"

        "src/CodeEmitterContext.cpp"
)

EzCMK_ConfigureLib(NAME "EzCodeEmitter"
        FILE_LIST ${EZCODEEMITTER_FILES_LIST}
        PRECOMPILED_HEADER "include/EzCodeEmitterCommon.h"
        LINKED_LIBRARIES "EzMir"
        INCLUDED_DIRS "include")
