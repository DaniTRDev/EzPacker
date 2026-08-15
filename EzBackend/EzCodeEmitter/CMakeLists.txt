set(EZCODEEMITTER_FILES_LIST
        "include/CodeEmitter.h"
        "include/CodeEmitterRuleBuilder.h"
        "include/EzCodeEmitter.h"
        "include/EzCodeEmitterCommon.h"
        "include/GenericCodeEmitter.h"

        "src/CodeEmitter.cpp"
        "src/CodeEmitterRuleBuilder.cpp"
)

EzCMK_ConfigureLib(NAME "EzCodeEmitter"
        FILE_LIST ${EZCODEEMITTER_FILES_LIST}
        PRECOMPILED_HEADER "include/EzCodeEmitterCommon.h"
        LINKED_LIBRARIES "EzMir"
        INCLUDED_DIRS "include")
