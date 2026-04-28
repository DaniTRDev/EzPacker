set(EZABI_FILES_LIST
        "include/ABIDesc.h"
        "include/ArgLocation.h"
        "include/EzABI.h"
        "include/EzABICommon.h"
        "include/StackLayout.h"

        "src/ABIDesc.cpp"
        "src/ArgLocation.cpp"
        "src/StackLayout.cpp"
)

EzCMK_ConfigureLib(NAME "EzABI"
        FILE_LIST ${EZABI_FILES_LIST}
        PRECOMPILED_HEADER "include/EzABICommon.h"
        LINKED_LIBRARIES "EzCore;zasm"
        INCLUDED_DIRS "include")

if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzABI")
endif ()
