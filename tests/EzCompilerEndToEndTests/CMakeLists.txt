cmake_minimum_required(VERSION 3.27)

# Helper tool to validate generated Linux ELF64 relocatable objects
add_executable(elf_validator elf_validator.c)

set(E2E_TESTS
    01_arithmetic_logic
    02_branch_and_loops
    03_calling_conventions
    04_memory_access
    05_recursion_and_calls
    06_phi_and_control_flow
)

set(ALL_E2E_OUTPUTS "")
set(EZC_BIN "$<TARGET_FILE:ezc>")

get_filename_component(COMPILER_DIR "${CMAKE_C_COMPILER}" DIRECTORY)

foreach(TEST_NAME IN LISTS E2E_TESTS)
    set(MIR_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}.mir")
    set(C_TEST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME}_test.c")
    set(WIN_OBJ "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}_win.obj")
    set(LINUX_OBJ "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}_linux.o")
    set(TEST_EXE "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}_test${CMAKE_EXECUTABLE_SUFFIX}")

    # 1. Compile MIR file to Windows x64 COFF object
    add_custom_command(
        OUTPUT "${WIN_OBJ}"
        COMMAND "${EZC_BIN}" --target x86_64-pc-windows-coff "${MIR_FILE}" -o "${WIN_OBJ}"
        DEPENDS ezc "${MIR_FILE}"
        COMMENT "[EzCompiler E2E] Compiling ${TEST_NAME}.mir -> ${TEST_NAME}_win.obj (x86_64 COFF)"
        VERBATIM
    )

    # 2. Compile MIR file to Linux x64 ELF object
    add_custom_command(
        OUTPUT "${LINUX_OBJ}"
        COMMAND "${EZC_BIN}" --target x86_64-linux-gnu "${MIR_FILE}" -o "${LINUX_OBJ}"
        DEPENDS ezc "${MIR_FILE}"
        COMMENT "[EzCompiler E2E] Compiling ${TEST_NAME}.mir -> ${TEST_NAME}_linux.o (x86_64 ELF)"
        VERBATIM
    )

    # 3. Compile and link C test harness with the generated Windows object file
    add_custom_command(
        OUTPUT "${TEST_EXE}"
        COMMAND "${CMAKE_C_COMPILER}" "${C_TEST_FILE}" "${WIN_OBJ}" -o "${TEST_EXE}"
        DEPENDS "${C_TEST_FILE}" "${WIN_OBJ}"
        COMMENT "[EzCompiler E2E] Linking ${TEST_NAME}_test (${TEST_NAME}_test.c + ${TEST_NAME}_win.obj)"
        VERBATIM
    )

    list(APPEND ALL_E2E_OUTPUTS "${WIN_OBJ}" "${LINUX_OBJ}" "${TEST_EXE}")

    # 4. Register CTest runtime test for Windows
    add_test(
        NAME "EzCompilerEndToEnd_${TEST_NAME}"
        COMMAND "${TEST_EXE}"
    )

    # 5. Register CTest ELF validation test for Linux
    add_test(
        NAME "EzCompilerEndToEnd_${TEST_NAME}_ElfValidate"
        COMMAND elf_validator "${LINUX_OBJ}"
    )

    if (WIN32)
        set_tests_properties("EzCompilerEndToEnd_${TEST_NAME}" PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin;PATH=path_list_prepend:${COMPILER_DIR}"
        )
        set_tests_properties("EzCompilerEndToEnd_${TEST_NAME}_ElfValidate" PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin;PATH=path_list_prepend:${COMPILER_DIR}"
        )
    endif ()
endforeach()

# Master CMake target for the end-to-end test suite
add_custom_target(EzCompilerEndToEndTests ALL
    DEPENDS ${ALL_E2E_OUTPUTS} elf_validator
)
