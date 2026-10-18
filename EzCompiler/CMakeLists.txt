set(EZCOMPILER_LIB_FILES_LIST
        "include/EzCompilerCommon.h"
        "include/CommandLineOptions.h"
        "include/DriverContext.h"
        "include/TargetResolver.h"
        "include/TargetTriple.h"
        "include/FrontendAdapter.h"
        "include/CompilationPipeline.h"
        "include/EmissionEngine.h"
        "include/CFrontend/ChibiccWrapper.h"
        "include/CFrontend/CFrontendTypeMapper.h"
        "include/CFrontend/CFrontendFunctionLowerer.h"
        "include/CFrontend/CFrontendAdapter.h"

        "src/CommandLineOptions.cpp"
        "src/DriverContext.cpp"
        "src/TargetResolver.cpp"
        "src/TargetTriple.cpp"
        "src/FrontendAdapter.cpp"
        "src/CompilationPipeline.cpp"
        "src/EmissionEngine.cpp"
        "src/CFrontend/ChibiccWrapper.cpp"
        "src/CFrontend/CFrontendTypeMapper.cpp"
        "src/CFrontend/CFrontendFunctionLowerer.cpp"
        "src/CFrontend/CFrontendAdapter.cpp"
)

EzCMK_ConfigureLib(NAME "EzCompilerLib"
        FILE_LIST ${EZCOMPILER_LIB_FILES_LIST}
        PRECOMPILED_HEADER "include/EzCompilerCommon.h"
        LINKED_LIBRARIES "EzTriple;EzMir;EzCore;EzCodeEmitter;argparse;chibicc_frontend"
        INCLUDED_DIRS "include;../EzCodeEmitter/include;../EzTriple/include;../EzMir/include;../EzCore/include;${chibicc_SOURCE_DIR};${chibicc_SOURCE_DIR}/shims"
)

EzCMK_AddExecutable(NAME "ezc"
        FILE_LIST "src/Main.cpp"
        PRECOMPILED_HEADER "include/EzCompilerCommon.h"
        LINKED_LIBRARIES "EzCompilerLib;EzTargetsX86_64Registration;EzTargetsRiscV64Registration"
        INCLUDED_DIRS "include;${CMAKE_SOURCE_DIR}/EzTargets/X86_64/Registration/include;${CMAKE_SOURCE_DIR}/EzTargets/RiscV64/Registration/include"
)
