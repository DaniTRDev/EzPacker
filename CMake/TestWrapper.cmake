set(EZPACKER_TEST_FILES "" CACHE STRING "Testing target source files list" FORCE)
set(EZPACKER_TEST_LIBRARIES "" CACHE STRING "Libraries used by testing targets" FORCE)

function(EzPacker_AddTest NAME TESTED_TARGET LIBRARIES FILES)
    set(FIXED_FILE_LIST "") # Make the file list have the full absolute path.
    foreach (FILE IN LISTS FILES)
        set(FIXED_FILE_LIST ${FIXED_FILE_LIST} ${CMAKE_CURRENT_SOURCE_DIR}/${FILE})
    endforeach ()

    EzCMK_AddTest(TEST_NAME ${NAME}
            TESTED_TARGET "${TESTED_TARGET}"
            TEST_FILES "${FIXED_FILE_LIST}"
            TEST_LIBRARIES "${LIBRARIES};gtest")

    set(EZPACKER_TEST_FILES ${EZPACKER_TEST_FILES} ${FIXED_FILE_LIST} CACHE STRING "Testing files" FORCE)
    set(EZPACKER_TEST_LIBRARIES ${EZPACKER_TEST_LIBRARIES} "${TESTED_TARGET};${LIBRARIES};" CACHE STRING "Testing libraries used by tests" FORCE)
endfunction()

function(EzPacker_BuildTests MAIN_FILE_PATH)
    message(STATUS "Building test target TEST")

    EzCMK_AddExecutable(NAME "TEST_ALL"
            FILE_LIST "${MAIN_FILE_PATH};${EZPACKER_TEST_FILES}"
            LINKED_LIBRARIES "${EZPACKER_TEST_LIBRARIES};gtest"
    )

    set_target_properties("TEST_ALL" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endfunction()