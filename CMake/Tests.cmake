function(EzPacker_AddTest testName testDefinitions testLibraries testDependencies testFiles)
    add_executable("${testName}" ${testFiles})

    if (testDefinitions)
        target_compile_definitions("${testName}" PUBLIC ${testDefinitions})
    endif ()

    target_link_libraries("${testName}" PUBLIC ${testLibraries})

    if (testDependencies)
        add_dependencies("${testName}" ${testDependencies})
    endif ()

    message(STATUS "Adding test ${testName}")
    add_test("${testName}" "${testName}")
endfunction()