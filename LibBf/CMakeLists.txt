# CMake configurator for https://bellard.org/libbf/

set(LIBBF_FILES_LIST

        "src/cutils.c"
        "src/libbf.c"
        "src/softfp.c"
)

EzCMK_ConfigureLib(NAME "LibBF"
        FILE_LIST ${LIBBF_FILES_LIST}
        PRECOMPILED_HEADER "include/libbf.h"
        LINKED_LIBRARIES ""
        INCLUDED_DIRS "include"
)

target_compile_definitions(LibBF PRIVATE
        LIMB_LOG2=64
)

target_compile_features(LibBF PRIVATE c_std_99)