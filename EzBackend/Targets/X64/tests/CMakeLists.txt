if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for X64")

    EzPacker_AddTest(
            "T_X64Flow"
            "Target_x64;EzTarget;EzMir"
            ""
            "T_X64Flow.cpp"
    )
endif ()
