set(EZDSL_CLI_FILES_LIST
        "include/EzDslCommon.h"
    
        "include/CodeGenerators/CppMirInstructionGenerator.h"
        "include/CodeGenerators/CppMirTypeTableGenerator.h"
        "include/CodeGenerators/CppTargetBankGenerator.h"

        "src/CodeGenerators/CppMirInstructionGenerator.cpp"
        "src/CodeGenerators/CppMirTypeTableGenerator.cpp"
        "src/CodeGenerators/CppTargetBankGenerator.cpp"
        
        
)

EzCMK_AddExecutable(NAME "EzDslCli"
        FILE_LIST ${EZDSL_FILES_LIST}
        PRECOMPILED_HEADER "include/EzDslCliCommon.h"
        LINKED_LIBRARIES "EzDslSema"
        INCLUDED_DIRS "include"
)