set(EZDSL_CLI_LIB_FILES_LIST
        "include/EzDslCliCommon.h"
        "include/Cli/CommandLineOptions.h"
        "include/Cli/Driver.h"
        "include/Cli/InfoDumper.h"

        "src/Cli/CommandLineOptions.cpp"
        "src/Cli/Driver.cpp"
        "src/Cli/InfoDumper.cpp"
)

EzCMK_ConfigureLib(NAME "EzDslCliLib"
        FILE_LIST ${EZDSL_CLI_LIB_FILES_LIST}
        PRECOMPILED_HEADER "include/EzDslCliCommon.h"
        LINKED_LIBRARIES "EzDslCodeGenerators;EzDslSema;EzDslLexer;EzCore;argparse"
        INCLUDED_DIRS "include"
)

EzCMK_AddExecutable(NAME "EzDslCli"
        FILE_LIST "src/Main.cpp"
        PRECOMPILED_HEADER "include/EzDslCliCommon.h"
        LINKED_LIBRARIES "EzDslCliLib"
        INCLUDED_DIRS "include"
)