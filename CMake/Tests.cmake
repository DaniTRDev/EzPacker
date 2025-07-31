set(EZPACKER_TEST_FILES "" CACHE STRING "Testing target source files list" FORCE)
set(EZPACKER_TEST_LIBRARIES "" CACHE STRING "Libraries used by testing targets" FORCE)

function(EzPacker_AddTest TestName TestDefinitions TestLibraries TestDependencies TestFiles)
    set(FixedTestFileList "") # Make the file list have the full absolute path.
    foreach (file IN LISTS TestFiles)
        set(FixedTestFileList ${FixedTestFileList} ${CMAKE_CURRENT_SOURCE_DIR}/${file})
    endforeach ()

    add_executable(${TestName} ${FixedTestFileList})

    if (testDefinitions)
        target_compile_definitions(${TestName} PUBLIC ${TestDefinitions})
    endif ()

    target_link_libraries(${TestName} PUBLIC ${TestLibraries} gtest gtest_main)
    target_include_directories(${TestName} PUBLIC ${gtest_build_include_dirs})
    set_target_properties(${TestName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

    if (TestDependencies)
        add_dependencies(${TestName} ${TestDependencies})
    endif ()

    message(STATUS "Adding test ${TestName}")

    set(EZPACKER_TEST_FILES ${EZPACKER_TEST_FILES} ${FixedTestFileList} CACHE STRING "Testing files" FORCE)
    set(EZPACKER_TEST_LIBRARIES ${EZPACKER_TEST_LIBRARIES} ${TestLibraries} CACHE STRING "Testing libraries used by tests" FORCE)

    add_test(${TestName} ${CMAKE_BINARY_DIR}/bin/${TestName})
endfunction()

function(EzPacker_BuildTests MainFilePath)
    message(STATUS "Building test target TEST")
    add_executable("T_TestAll" "${MainFilePath}" ${EZPACKER_TEST_FILES})

    target_link_libraries("T_TestAll" PUBLIC ${EZPACKER_TEST_LIBRARIES} gtest gtest_main)
    set_target_properties("T_TestAll" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endfunction()