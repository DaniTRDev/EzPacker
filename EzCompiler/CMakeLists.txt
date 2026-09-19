set(EZCOMPILER_LIB_FILES_LIST
        "include/EzCompilerCommon.h"
        "include/CommandLineOptions.h"
        "include/DriverContext.h"
        "include/TargetResolver.h"
        "include/TargetTriple.h"
        "include/FrontendAdapter.h"
        "include/CompilationPipeline.h"
        "include/EmissionEngine.h"

        "src/CommandLineOptions.cpp"
        "src/DriverContext.cpp"
        "src/TargetResolver.cpp"
        "src/TargetTriple.cpp"
        "src/FrontendAdapter.cpp"
        "src/CompilationPipeline.cpp"
        "src/EmissionEngine.cpp"
        "src/Targets/X86_64TargetRegistration.cpp"
)

EzCMK_ConfigureLib(NAME "EzCompilerLib"
        FILE_LIST ${EZCOMPILER_LIB_FILES_LIST}
        PRECOMPILED_HEADER "include/EzCompilerCommon.h"
        LINKED_LIBRARIES "EzTriple;EzMir;EzCore;EzCodeEmitter;argparse"
        INCLUDED_DIRS "include;../EzCodeEmitter/include;../EzTriple/include;../EzMir/include;../EzCore/include"
)

EzCMK_AddExecutable(NAME "ezc"
        FILE_LIST "src/Main.cpp"
        PRECOMPILED_HEADER "include/EzCompilerCommon.h"
        LINKED_LIBRARIES "EzCompilerLib"
        INCLUDED_DIRS "include"
)
