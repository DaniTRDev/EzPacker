function(EzPacker_AddTest testName testDefinitions testDependencies testFiles)
    add_executable("${testName}" ${testFiles})
    target_compile_definitions("${testName}" PUBLIC ${testDefinitions})
    target_link_libraries("${testName}" PUBLIC ${testDependencies})

    message(STATUS "Adding test ${testName}")
    add_test("${testName}" "${testName}")
endfunction()