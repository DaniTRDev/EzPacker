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

    if (WIN32)
        get_filename_component(COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
        set_tests_properties(${NAME} PROPERTIES ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin;PATH=path_list_prepend:${COMPILER_DIR}")
    endif ()

    if (TARGET EzPacker_CopyStandardLibraries)
        set(TEST_TARGET_NAME ${TESTED_TARGET}_${NAME})
        if (TARGET ${TEST_TARGET_NAME})
            add_dependencies(${TEST_TARGET_NAME} EzPacker_CopyStandardLibraries)
        endif ()
    endif ()

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
    if (TARGET EzPacker_CopyStandardLibraries AND TARGET "TEST_ALL")
        add_dependencies("TEST_ALL" EzPacker_CopyStandardLibraries)
    endif ()
endfunction()