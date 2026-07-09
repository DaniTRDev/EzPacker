set(EZCORE_FILES_LIST
        "include/EzCore.h"
        "include/EzCoreCommon.h"
        "include/StringUtils.h"
        "include/Diagnostics/DiagnosticBuilder.h"
        "include/Diagnostics/DiagnosticCollector.h"
        "include/Diagnostics/DiagnosticListener.h"
        "include/Diagnostics/DiagnosticLogger.h"
        "include/Diagnostics/DiagnosticMessage.h"
        "include/Diagnostics/DiagnosticScope.h"
        "include/FlexNumber/FlexFloat.h"
        "include/FlexNumber/FlexInt.h"
        "include/SourceManager/SourceManager.h"

        "src/Diagnostics/DiagnosticBuilder.cpp"
        "src/Diagnostics/DiagnosticCollector.cpp"
        "src/Diagnostics/DiagnosticLogger.cpp"
        "src/Diagnostics/DiagnosticMessage.cpp"
        "src/Diagnostics/DiagnosticScope.cpp"
        "src/FlexNumber/FlexFloat.cpp"
        "src/FlexNumber/FlexInt.cpp"
        "src/SourceManager/SourceManager.cpp"
)

EzCMK_ConfigureLib(NAME "EzCore"
        FILE_LIST ${EZCORE_FILES_LIST}
        PRECOMPILED_HEADER "include/EzCoreCommon.h"
        LINKED_LIBRARIES "EzLogger;libtommath;LibBF;gtest;GTest::gtest_main"
        INCLUDED_DIRS "include;${gtest_build_include_dirs}"
)

if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzCore")

    EzPacker_AddTest(
            "T_DiagnosticLogger"
            "EzCore"
            ""
            "tests/T_DiagnosticLogger.cpp"
    )
endif ()
