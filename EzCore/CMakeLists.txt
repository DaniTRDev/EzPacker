set(EZCORE_FILES_LIST
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
        "include/HelperClasses/DenseBitSet.h"
        "include/HelperClasses/IntrusiveLinkedList.h"
        "include/SourceManager/GenericSourceManager.h"
        "include/SourceManager/SourceManager.h"

        "src/Diagnostics/DiagnosticBuilder.cpp"
        "src/Diagnostics/DiagnosticCollector.cpp"
        "src/Diagnostics/DiagnosticLogger.cpp"
        "src/Diagnostics/DiagnosticMessage.cpp"
        "src/Diagnostics/DiagnosticScope.cpp"
        "src/FlexNumber/FlexFloat.cpp"
        "src/FlexNumber/FlexInt.cpp"
        "src/HelperClasses/DenseBitSet.cpp"
        "src/SourceManager/SourceManager.cpp"
)

EzCMK_ConfigureLib(NAME "EzCore"
        FILE_LIST ${EZCORE_FILES_LIST}
        PRECOMPILED_HEADER "include/EzCoreCommon.h"
        LINKED_LIBRARIES "EzLogger;libtommath;LibBF"
        INCLUDED_DIRS "include"
)

if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzCore")

    EzPacker_AddTest(
            "T_DiagnosticLogger"
            "EzCore"
            "gtest;GTest::gtest_main"
            "tests/T_DiagnosticLogger.cpp"
    )

    EzPacker_AddTest(
            "T_IntrusiveLinkedList"
            "EzCore"
            "gtest;GTest::gtest_main"
            "tests/T_IntrusiveLinkedList.cpp"
    )

    EzPacker_AddTest(
            "T_DenseBitSet"
            "EzCore"
            "gtest;GTest::gtest_main"
            "tests/T_DenseBitSet.cpp"
    )

    EzPacker_AddTest(
            "T_SourceManager"
            "EzCore"
            "gtest;GTest::gtest_main"
            "tests/T_SourceManager.cpp"
    )
endif ()
