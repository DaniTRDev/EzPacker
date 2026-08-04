add_subdirectory(EzMir)
add_subdirectory(EzTriple)
add_subdirectory(EzTriples)

if (EZPACKER_BUILD_TESTS)
    message(STATUS "Configuring EzMirTestSuite")
    add_subdirectory(EzMirTestSuite)

    message(STATUS "Configuring EzTripleTestSuite")
    add_subdirectory(EzTripleTestSuite)
endif ()