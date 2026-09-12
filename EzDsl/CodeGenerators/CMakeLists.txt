set(EZDSL_CODE_GENERATORS_FILES_LIST
        "include/EzDslCodeGeneratorsCommon.h"
        "include/CodeGenerators/CppSourceEmitter.h"
        "include/CodeGenerators/CodeGenerator.h"
        "include/CodeGenerators/CppMirInstructionGenerator.h"
        "include/CodeGenerators/CppMirTypeTableGenerator.h"

        "src/CodeGenerators/CppSourceEmitter.cpp"
        "src/CodeGenerators/CodeGenerator.cpp"
        "src/CodeGenerators/CppMirInstructionGenerator.cpp"
        "src/CodeGenerators/CppMirTypeTableGenerator.cpp"
)

EzCMK_ConfigureLib(NAME "EzDslCodeGenerators"
        FILE_LIST ${EZDSL_CODE_GENERATORS_FILES_LIST}
        PRECOMPILED_HEADER "include/EzDslCodeGeneratorsCommon.h"
        LINKED_LIBRARIES "EzDslSema"
        INCLUDED_DIRS "include"
)