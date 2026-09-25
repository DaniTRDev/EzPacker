cmake_minimum_required(VERSION 3.27)

# Detect host operating system
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(HOST_IS_WINDOWS TRUE)
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    set(HOST_IS_LINUX TRUE)
else()
    message(WARNING "Host OS (${CMAKE_HOST_SYSTEM_NAME}) is neither Windows nor Linux. Defaulting to Linux.")
    set(HOST_IS_LINUX TRUE)
endif()

# Helper tool to validate generated ELF64 relocatable objects (built on Linux, or validated when cross-generating)
add_executable(elf_validator elf_validator.c)

set(E2E_TESTS
    01_arithmetic_logic
    02_branch_and_loops
    03_calling_conventions
    04_memory_access
    05_recursion_and_calls
    06_phi_and_control_flow
    07_floating_point
    08_comparisons
    09_bitwise_logic
    10_memory_and_aggregates
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
        COMMAND ${CMAKE_COMMAND} -E env "PATH=${CMAKE_BINARY_DIR}/bin;${COMPILER_DIR};$ENV{PATH}" "${EZC_BIN}" --target x86_64-pc-windows-coff "${MIR_FILE}" -o "${WIN_OBJ}"
        DEPENDS ezc "${MIR_FILE}"
        COMMENT "[EzCompiler E2E] Compiling ${TEST_NAME}.mir -> ${TEST_NAME}_win.obj (x86_64 COFF)"
        VERBATIM
    )

    # 2. Compile MIR file to Linux x64 ELF object
    add_custom_command(
        OUTPUT "${LINUX_OBJ}"
        COMMAND ${CMAKE_COMMAND} -E env "PATH=${CMAKE_BINARY_DIR}/bin;${COMPILER_DIR};$ENV{PATH}" "${EZC_BIN}" --target x86_64-linux-gnu "${MIR_FILE}" -o "${LINUX_OBJ}"
        DEPENDS ezc "${MIR_FILE}"
        COMMENT "[EzCompiler E2E] Compiling ${TEST_NAME}.mir -> ${TEST_NAME}_linux.o (x86_64 ELF)"
        VERBATIM
    )

    if(HOST_IS_WINDOWS)
        set(WIN_TEST_EXE "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}_win_test${CMAKE_EXECUTABLE_SUFFIX}")
        set(LINUX_TEST_EXE "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}_linux_test${CMAKE_EXECUTABLE_SUFFIX}")

        # 3a. Compile and link Windows test runner (Win64 ABI)
        add_custom_command(
            OUTPUT "${WIN_TEST_EXE}"
            COMMAND "${CMAKE_C_COMPILER}" -DABI_WIN64 "${C_TEST_FILE}" "${WIN_OBJ}" -o "${WIN_TEST_EXE}"
            DEPENDS "${C_TEST_FILE}" "${WIN_OBJ}"
            COMMENT "[EzCompiler E2E] Linking ${TEST_NAME}_win_test (${TEST_NAME}_test.c [Win64] + ${WIN_OBJ})"
            VERBATIM
        )

        # 3b. Compile and link Linux test runner (SysV ABI) via MinGW GCC
        add_custom_command(
            OUTPUT "${LINUX_TEST_EXE}"
            COMMAND "${CMAKE_C_COMPILER}" -DABI_SYSV "${C_TEST_FILE}" "${LINUX_OBJ}" -o "${LINUX_TEST_EXE}"
            DEPENDS "${C_TEST_FILE}" "${LINUX_OBJ}"
            COMMENT "[EzCompiler E2E] Linking ${TEST_NAME}_linux_test (${TEST_NAME}_test.c [SysV] + ${LINUX_OBJ})"
            VERBATIM
        )

        list(APPEND ALL_E2E_OUTPUTS "${WIN_OBJ}" "${LINUX_OBJ}" "${WIN_TEST_EXE}" "${LINUX_TEST_EXE}")

        # 4a. Register CTest runtime test for Win64 executable
        add_test(
            NAME "EzCompilerEndToEnd_${TEST_NAME}_Win64"
            COMMAND "${WIN_TEST_EXE}"
        )

        # 4b. Register CTest runtime test for SysV executable
        add_test(
            NAME "EzCompilerEndToEnd_${TEST_NAME}_SysV"
            COMMAND "${LINUX_TEST_EXE}"
        )

        # 5. Register ELF validator test
        add_test(
            NAME "EzCompilerEndToEnd_${TEST_NAME}_ElfValidate"
            COMMAND elf_validator "${LINUX_OBJ}"
        )

        set_tests_properties("EzCompilerEndToEnd_${TEST_NAME}_Win64" PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin;PATH=path_list_prepend:${COMPILER_DIR}"
        )
        set_tests_properties("EzCompilerEndToEnd_${TEST_NAME}_SysV" PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin;PATH=path_list_prepend:${COMPILER_DIR}"
        )
        set_tests_properties("EzCompilerEndToEnd_${TEST_NAME}_ElfValidate" PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin;PATH=path_list_prepend:${COMPILER_DIR}"
        )
    else()
        set(LINUX_TEST_EXE "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}_linux_test${CMAKE_EXECUTABLE_SUFFIX}")

        # 3. Compile and link Linux test runner (SysV ABI)
        add_custom_command(
            OUTPUT "${LINUX_TEST_EXE}"
            COMMAND "${CMAKE_C_COMPILER}" -DABI_SYSV "${C_TEST_FILE}" "${LINUX_OBJ}" -o "${LINUX_TEST_EXE}"
            DEPENDS "${C_TEST_FILE}" "${LINUX_OBJ}"
            COMMENT "[EzCompiler E2E] Linking ${TEST_NAME}_linux_test (${TEST_NAME}_test.c [SysV] + ${LINUX_OBJ})"
            VERBATIM
        )

        list(APPEND ALL_E2E_OUTPUTS "${WIN_OBJ}" "${LINUX_OBJ}" "${LINUX_TEST_EXE}")

        # 4. Register CTest runtime test for SysV executable
        add_test(
            NAME "EzCompilerEndToEnd_${TEST_NAME}_SysV"
            COMMAND "${LINUX_TEST_EXE}"
        )

        # 5. Register ELF validator test
        add_test(
            NAME "EzCompilerEndToEnd_${TEST_NAME}_ElfValidate"
            COMMAND elf_validator "${LINUX_OBJ}"
        )
    endif()
endforeach()

# Master CMake target for the end-to-end test suite
add_custom_target(EzCompilerEndToEndTests ALL
    DEPENDS ${ALL_E2E_OUTPUTS} elf_validator
)