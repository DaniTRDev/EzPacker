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

set(ALL_E2E_OUTPUTS "")
set(EZC_BIN "$<TARGET_FILE:ezc>")

get_filename_component(COMPILER_DIR "${CMAKE_C_COMPILER}" DIRECTORY)

# =========================================================================
# TIER 1: Redesigned YARP Freestanding End-to-End Test Suite (Zero stdlib)
# =========================================================================
set(YARP_E2E_TESTS
    yarp_01_arithmetic_logic
    yarp_02_branch_loops
    yarp_03_memory_arrays
    yarp_04_type_conversions
    yarp_05_structs_globals
    yarp_06_nested_calls
)

foreach(TEST_NAME IN LISTS YARP_E2E_TESTS)
    set(C_SRC_FILE "${CMAKE_CURRENT_SOURCE_DIR}/yarp_corpus/${TEST_NAME}.c")
    set(C_DRIVER_FILE "${CMAKE_CURRENT_SOURCE_DIR}/yarp_corpus/${TEST_NAME}_driver.c")
    set(WIN_OBJ "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}_win.obj")
    set(LINUX_OBJ "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}_linux.o")

    # 1. Compile YARP C source to Windows x64 COFF object
    add_custom_command(
        OUTPUT "${WIN_OBJ}"
        COMMAND ${CMAKE_COMMAND} -E env "PATH=${CMAKE_BINARY_DIR}/bin;${COMPILER_DIR};$ENV{PATH}" "${EZC_BIN}" -O2 --target x86_64-pc-windows-coff "${C_SRC_FILE}" -o "${WIN_OBJ}"
        DEPENDS ezc "${C_SRC_FILE}"
        COMMENT "[EzCompiler E2E / Yarp] Compiling ${TEST_NAME}.c -> ${TEST_NAME}_win.obj (x86_64 COFF)"
        VERBATIM
    )

    # 2. Compile YARP C source to Linux x64 ELF object
    add_custom_command(
        OUTPUT "${LINUX_OBJ}"
        COMMAND ${CMAKE_COMMAND} -E env "PATH=${CMAKE_BINARY_DIR}/bin;${COMPILER_DIR};$ENV{PATH}" "${EZC_BIN}" -O2 --target x86_64-linux-gnu "${C_SRC_FILE}" -o "${LINUX_OBJ}"
        DEPENDS ezc "${C_SRC_FILE}"
        COMMENT "[EzCompiler E2E / Yarp] Compiling ${TEST_NAME}.c -> ${TEST_NAME}_linux.o (x86_64 ELF)"
        VERBATIM
    )

    if(HOST_IS_WINDOWS)
        set(WIN_TEST_EXE "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}_win_test${CMAKE_EXECUTABLE_SUFFIX}")
        set(LINUX_TEST_EXE "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}_linux_test${CMAKE_EXECUTABLE_SUFFIX}")

        # 3a. Compile and link Windows test runner (Win64 ABI)
        add_custom_command(
            OUTPUT "${WIN_TEST_EXE}"
            COMMAND "${CMAKE_C_COMPILER}" -I "${CMAKE_CURRENT_SOURCE_DIR}/yarp_corpus" -DABI_WIN64 "${C_DRIVER_FILE}" "${WIN_OBJ}" -o "${WIN_TEST_EXE}"
            DEPENDS "${C_DRIVER_FILE}" "${WIN_OBJ}"
            COMMENT "[EzCompiler E2E / Yarp] Linking ${TEST_NAME}_win_test (${TEST_NAME}_driver.c [Win64] + ${WIN_OBJ})"
            VERBATIM
        )

        # 3b. Compile and link Linux test runner (SysV ABI) via MinGW GCC
        add_custom_command(
            OUTPUT "${LINUX_TEST_EXE}"
            COMMAND "${CMAKE_C_COMPILER}" -I "${CMAKE_CURRENT_SOURCE_DIR}/yarp_corpus" -DABI_SYSV "${C_DRIVER_FILE}" "${LINUX_OBJ}" -o "${LINUX_TEST_EXE}"
            DEPENDS "${C_DRIVER_FILE}" "${LINUX_OBJ}"
            COMMENT "[EzCompiler E2E / Yarp] Linking ${TEST_NAME}_linux_test (${TEST_NAME}_driver.c [SysV] + ${LINUX_OBJ})"
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
            LABELS "E2E;Yarp;Freestanding;EzCompiler"
        )
        set_tests_properties("EzCompilerEndToEnd_${TEST_NAME}_SysV" PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin;PATH=path_list_prepend:${COMPILER_DIR}"
            LABELS "E2E;Yarp;Freestanding;EzCompiler"
        )
        set_tests_properties("EzCompilerEndToEnd_${TEST_NAME}_ElfValidate" PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin;PATH=path_list_prepend:${COMPILER_DIR}"
            LABELS "E2E;ElfValidate;EzCompiler"
        )
    else()
        set(LINUX_TEST_EXE "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}_linux_test${CMAKE_EXECUTABLE_SUFFIX}")

        # 3. Compile and link Linux test runner (SysV ABI)
        add_custom_command(
            OUTPUT "${LINUX_TEST_EXE}"
            COMMAND "${CMAKE_C_COMPILER}" -I "${CMAKE_CURRENT_SOURCE_DIR}/yarp_corpus" -DABI_SYSV "${C_DRIVER_FILE}" "${LINUX_OBJ}" -o "${LINUX_TEST_EXE}"
            DEPENDS "${C_DRIVER_FILE}" "${LINUX_OBJ}"
            COMMENT "[EzCompiler E2E / Yarp] Linking ${TEST_NAME}_linux_test (${TEST_NAME}_driver.c [SysV] + ${LINUX_OBJ})"
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

        set_tests_properties("EzCompilerEndToEnd_${TEST_NAME}_SysV" PROPERTIES
            LABELS "E2E;Yarp;Freestanding;EzCompiler"
        )
        set_tests_properties("EzCompilerEndToEnd_${TEST_NAME}_ElfValidate" PROPERTIES
            LABELS "E2E;ElfValidate;EzCompiler"
        )
    endif()
endforeach()

# =========================================================================
# TIER 2: Dedicated Backend MIR End-to-End Tests (Exceptions & Vector SIMD)
# =========================================================================
set(MIR_BACKEND_TESTS
    12_exceptions
    13_vector_simd
)

foreach(TEST_NAME IN LISTS MIR_BACKEND_TESTS)
    set(MIR_FILE "${CMAKE_CURRENT_SOURCE_DIR}/mir_backend/${TEST_NAME}.mir")
    set(C_TEST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/mir_backend/${TEST_NAME}_test.c")
    set(WIN_OBJ "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}_win.obj")
    set(LINUX_OBJ "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}_linux.o")

    if(TEST_NAME STREQUAL "12_exceptions")
        set(WIN_EXTRA_LINK_FLAGS "$<TARGET_FILE:EzExceptionRuntime>" -lstdc++)
        set(WIN_EXTRA_DEPENDS EzExceptionRuntime)
        if(HOST_IS_WINDOWS)
            set(LINUX_EXTRA_LINK_FLAGS "")
            set(LINUX_EXTRA_DEPENDS "")
        else()
            set(LINUX_EXTRA_LINK_FLAGS "$<TARGET_FILE:EzExceptionRuntime>" -lstdc++)
            set(LINUX_EXTRA_DEPENDS EzExceptionRuntime)
        endif()
        set(EXTRA_EZC_FLAGS "")
        set(EXTRA_C_FLAGS -I "${CMAKE_CURRENT_SOURCE_DIR}" -I "${CMAKE_SOURCE_DIR}/EzLinker/runtime")
    elseif(TEST_NAME STREQUAL "13_vector_simd")
        set(WIN_EXTRA_LINK_FLAGS "")
        set(WIN_EXTRA_DEPENDS "")
        set(LINUX_EXTRA_LINK_FLAGS "")
        set(LINUX_EXTRA_DEPENDS "")
        set(EXTRA_EZC_FLAGS -mattr +avx2)
        set(EXTRA_C_FLAGS -mavx2 -I "${CMAKE_CURRENT_SOURCE_DIR}")
    else()
        set(WIN_EXTRA_LINK_FLAGS "")
        set(WIN_EXTRA_DEPENDS "")
        set(LINUX_EXTRA_LINK_FLAGS "")
        set(LINUX_EXTRA_DEPENDS "")
        set(EXTRA_EZC_FLAGS "")
        set(EXTRA_C_FLAGS -I "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    # 1. Compile MIR file to Windows x64 COFF object
    add_custom_command(
        OUTPUT "${WIN_OBJ}"
        COMMAND ${CMAKE_COMMAND} -E env "PATH=${CMAKE_BINARY_DIR}/bin;${COMPILER_DIR};$ENV{PATH}" "${EZC_BIN}" ${EXTRA_EZC_FLAGS} --target x86_64-pc-windows-coff "${MIR_FILE}" -o "${WIN_OBJ}"
        DEPENDS ezc "${MIR_FILE}"
        COMMENT "[EzCompiler E2E / Backend] Compiling ${TEST_NAME}.mir -> ${TEST_NAME}_win.obj (x86_64 COFF)"
        VERBATIM
    )

    # 2. Compile MIR file to Linux x64 ELF object
    add_custom_command(
        OUTPUT "${LINUX_OBJ}"
        COMMAND ${CMAKE_COMMAND} -E env "PATH=${CMAKE_BINARY_DIR}/bin;${COMPILER_DIR};$ENV{PATH}" "${EZC_BIN}" ${EXTRA_EZC_FLAGS} --target x86_64-linux-gnu "${MIR_FILE}" -o "${LINUX_OBJ}"
        DEPENDS ezc "${MIR_FILE}"
        COMMENT "[EzCompiler E2E / Backend] Compiling ${TEST_NAME}.mir -> ${TEST_NAME}_linux.o (x86_64 ELF)"
        VERBATIM
    )

    if(HOST_IS_WINDOWS)
        set(WIN_TEST_EXE "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}_win_test${CMAKE_EXECUTABLE_SUFFIX}")
        set(LINUX_TEST_EXE "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}_linux_test${CMAKE_EXECUTABLE_SUFFIX}")

        add_custom_command(
            OUTPUT "${WIN_TEST_EXE}"
            COMMAND "${CMAKE_C_COMPILER}" -DABI_WIN64 ${EXTRA_C_FLAGS} "${C_TEST_FILE}" "${WIN_OBJ}" ${WIN_EXTRA_LINK_FLAGS} -o "${WIN_TEST_EXE}"
            DEPENDS "${C_TEST_FILE}" "${WIN_OBJ}" ${WIN_EXTRA_DEPENDS}
            COMMENT "[EzCompiler E2E / Backend] Linking ${TEST_NAME}_win_test (${TEST_NAME}_test.c [Win64] + ${WIN_OBJ})"
            VERBATIM
        )

        add_custom_command(
            OUTPUT "${LINUX_TEST_EXE}"
            COMMAND "${CMAKE_C_COMPILER}" -DABI_SYSV ${EXTRA_C_FLAGS} "${C_TEST_FILE}" "${LINUX_OBJ}" ${LINUX_EXTRA_LINK_FLAGS} -o "${LINUX_TEST_EXE}"
            DEPENDS "${C_TEST_FILE}" "${LINUX_OBJ}" ${LINUX_EXTRA_DEPENDS}
            COMMENT "[EzCompiler E2E / Backend] Linking ${TEST_NAME}_linux_test (${TEST_NAME}_test.c [SysV] + ${LINUX_OBJ})"
            VERBATIM
        )

        list(APPEND ALL_E2E_OUTPUTS "${WIN_OBJ}" "${LINUX_OBJ}" "${WIN_TEST_EXE}" "${LINUX_TEST_EXE}")

        add_test(
            NAME "EzCompilerEndToEnd_${TEST_NAME}_Win64"
            COMMAND "${WIN_TEST_EXE}"
        )

        add_test(
            NAME "EzCompilerEndToEnd_${TEST_NAME}_SysV"
            COMMAND "${LINUX_TEST_EXE}"
        )

        add_test(
            NAME "EzCompilerEndToEnd_${TEST_NAME}_ElfValidate"
            COMMAND elf_validator "${LINUX_OBJ}"
        )

        set_tests_properties("EzCompilerEndToEnd_${TEST_NAME}_Win64" PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin;PATH=path_list_prepend:${COMPILER_DIR}"
            LABELS "E2E;Backend;Mir;EzCompiler"
        )
        set_tests_properties("EzCompilerEndToEnd_${TEST_NAME}_SysV" PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin;PATH=path_list_prepend:${COMPILER_DIR}"
            LABELS "E2E;Backend;Mir;EzCompiler"
        )
        set_tests_properties("EzCompilerEndToEnd_${TEST_NAME}_ElfValidate" PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin;PATH=path_list_prepend:${COMPILER_DIR}"
            LABELS "E2E;ElfValidate;EzCompiler"
        )
    else()
        set(LINUX_TEST_EXE "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}_linux_test${CMAKE_EXECUTABLE_SUFFIX}")

        add_custom_command(
            OUTPUT "${LINUX_TEST_EXE}"
            COMMAND "${CMAKE_C_COMPILER}" -DABI_SYSV ${EXTRA_C_FLAGS} "${C_TEST_FILE}" "${LINUX_OBJ}" ${LINUX_EXTRA_LINK_FLAGS} -o "${LINUX_TEST_EXE}"
            DEPENDS "${C_TEST_FILE}" "${LINUX_OBJ}" ${LINUX_EXTRA_DEPENDS}
            COMMENT "[EzCompiler E2E / Backend] Linking ${TEST_NAME}_linux_test (${TEST_NAME}_test.c [SysV] + ${LINUX_OBJ})"
            VERBATIM
        )

        list(APPEND ALL_E2E_OUTPUTS "${WIN_OBJ}" "${LINUX_OBJ}" "${LINUX_TEST_EXE}")

        add_test(
            NAME "EzCompilerEndToEnd_${TEST_NAME}_SysV"
            COMMAND "${LINUX_TEST_EXE}"
        )

        add_test(
            NAME "EzCompilerEndToEnd_${TEST_NAME}_ElfValidate"
            COMMAND elf_validator "${LINUX_OBJ}"
        )

        set_tests_properties("EzCompilerEndToEnd_${TEST_NAME}_SysV" PROPERTIES
            LABELS "E2E;Backend;Mir;EzCompiler"
        )
        set_tests_properties("EzCompilerEndToEnd_${TEST_NAME}_ElfValidate" PROPERTIES
            LABELS "E2E;ElfValidate;EzCompiler"
        )
    endif()
endforeach()

# =========================================================================
# TIER 3: C11 Hosted End-to-End Tests (EzMusl + ezc + ez-ld)
# =========================================================================
set(C11_HOSTED_TESTS
    test_exec
    test_arr
    test_malloc
    test_sz
    test_field
    test_div
    test_qsort
    test_debug
    test_stdio_format
    test_stdio_file_io
    test_stdlib_alloc_conv
    test_stdlib_sort_search
    test_string_mem
    test_ctype_errno
    test_time_assert
)

set(EZLD_BIN "$<TARGET_FILE:ez-ld>")

if(HOST_IS_WINDOWS)
    set(HOSTED_OBJ_EXT ".obj")
    set(HOSTED_ENV --modify "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin" --modify "PATH=path_list_prepend:${COMPILER_DIR}")
    set(HOSTED_LINK_EXTRA_DEPS "")
    if(TARGET EzPacker_CopyStandardLibraries)
        list(APPEND HOSTED_LINK_EXTRA_DEPS EzPacker_CopyStandardLibraries)
    endif()
    set(HOSTED_RUN_ENV "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin;PATH=path_list_prepend:${COMPILER_DIR}")
else()
    set(HOSTED_OBJ_EXT ".o")
    set(HOSTED_ENV --modify "LD_LIBRARY_PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin" --modify "PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin")
    set(HOSTED_LINK_EXTRA_DEPS EzMusl_crt1)
    set(HOSTED_RUN_ENV "LD_LIBRARY_PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin;PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin")
endif()

function(ezpacker_add_hosted_e2e_target TARGET_ID C_SRC_PATH)
    cmake_parse_arguments(ARG "" "OUT_EXE_VAR" "" ${ARGN})
    set(HOSTED_OBJ "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_ID}_hosted${HOSTED_OBJ_EXT}")
    set(HOSTED_EXE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_ID}_hosted${CMAKE_EXECUTABLE_SUFFIX}")

    add_custom_command(
        OUTPUT "${HOSTED_OBJ}"
        COMMAND ${CMAKE_COMMAND} -E env ${HOSTED_ENV} "${EZC_BIN}" -fhosted "${C_SRC_PATH}" -o "${HOSTED_OBJ}"
        DEPENDS ezc EzMusl_StageHeaders "${C_SRC_PATH}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "[EzCompiler E2E / Hosted] Compiling ${TARGET_ID} with ezc -fhosted"
        VERBATIM
    )

    add_custom_command(
        OUTPUT "${HOSTED_EXE}"
        COMMAND ${CMAKE_COMMAND} -E env ${HOSTED_ENV} "${EZLD_BIN}" --hosted --no-exception-rt --musl-path "$<TARGET_FILE:EzMusl>" "${HOSTED_OBJ}" -o "${HOSTED_EXE}"
        DEPENDS ez-ld EzMusl ${HOSTED_LINK_EXTRA_DEPS} "${HOSTED_OBJ}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "[EzCompiler E2E / Hosted] Linking ${TARGET_ID} with ez-ld --hosted (EzMusl)"
        VERBATIM
    )

    list(APPEND ALL_E2E_OUTPUTS "${HOSTED_OBJ}" "${HOSTED_EXE}")
    set(ALL_E2E_OUTPUTS "${ALL_E2E_OUTPUTS}" PARENT_SCOPE)
    if(ARG_OUT_EXE_VAR)
        set(${ARG_OUT_EXE_VAR} "${HOSTED_EXE}" PARENT_SCOPE)
    endif()
endfunction()

foreach(TEST_NAME IN LISTS C11_HOSTED_TESTS)
    set(C_SRC_FILE "${CMAKE_CURRENT_SOURCE_DIR}/c11_hosted/${TEST_NAME}.c")
    ezpacker_add_hosted_e2e_target("${TEST_NAME}" "${C_SRC_FILE}" OUT_EXE_VAR HOSTED_EXE)

    add_test(
        NAME "EzCompilerEndToEnd_Hosted_${TEST_NAME}"
        COMMAND "${HOSTED_EXE}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )

    set_tests_properties("EzCompilerEndToEnd_Hosted_${TEST_NAME}" PROPERTIES
        ENVIRONMENT_MODIFICATION "${HOSTED_RUN_ENV}"
        LABELS "E2E;Hosted;C11;Musl;EzCompiler"
    )
endforeach()

# CRT argc/argv + atexit LIFO ordering test
ezpacker_add_hosted_e2e_target("test_crt_args_exit" "${CMAKE_CURRENT_SOURCE_DIR}/c11_hosted/test_crt_args_exit.c" OUT_EXE_VAR ARGS_EXE)
add_test(
    NAME "EzCompilerEndToEnd_Hosted_test_crt_args_exit"
    COMMAND "${ARGS_EXE}" "alpha" "beta"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
set_tests_properties("EzCompilerEndToEnd_Hosted_test_crt_args_exit" PROPERTIES
    ENVIRONMENT_MODIFICATION "${HOSTED_RUN_ENV}"
    PASS_REGULAR_EXPRESSION "argc=3 arg1=alpha arg2=beta.*atexit_first.*atexit_second"
    LABELS "E2E;Hosted;C11;Musl;CRT;EzCompiler"
)

# CRT exit(42) + atexit cleanup propagation test
ezpacker_add_hosted_e2e_target("test_crt_exit_code" "${CMAKE_CURRENT_SOURCE_DIR}/c11_hosted/test_crt_exit_code.c" OUT_EXE_VAR EXIT_CODE_EXE)
add_test(
    NAME "EzCompilerEndToEnd_Hosted_test_crt_exit_code"
    COMMAND ${CMAKE_COMMAND}
        "-DTEST_EXE=${EXIT_CODE_EXE}"
        "-DEXPECTED_EXIT_CODE=42"
        "-DEXPECTED_OUTPUT_SUBSTR=exit_code_atexit_ran"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/VerifyExitCode.cmake"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
set_tests_properties("EzCompilerEndToEnd_Hosted_test_crt_exit_code" PROPERTIES
    ENVIRONMENT_MODIFICATION "${HOSTED_RUN_ENV}"
    LABELS "E2E;Hosted;C11;Musl;CRT;EzCompiler"
)

# Real-world hosted example pipeline test
ezpacker_add_hosted_e2e_target("example_10_hosted_c11_pipeline" "${CMAKE_SOURCE_DIR}/examples/10_hosted_c11_pipeline.c" OUT_EXE_VAR EX10_EXE)
add_test(
    NAME "EzCompilerEndToEnd_Hosted_example_10_pipeline"
    COMMAND "${EX10_EXE}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
set_tests_properties("EzCompilerEndToEnd_Hosted_example_10_pipeline" PROPERTIES
    ENVIRONMENT_MODIFICATION "${HOSTED_RUN_ENV}"
    PASS_REGULAR_EXPRESSION "Pipeline test completed successfully"
    LABELS "E2E;Hosted;C11;Musl;Examples;EzCompiler"
)

# Cross-platform POSIX + musl syscall tests (unistd, fcntl, fdopen, stat, unlink)
ezpacker_add_hosted_e2e_target("test_posix_musl" "${CMAKE_CURRENT_SOURCE_DIR}/c11_hosted/test_posix_musl.c" OUT_EXE_VAR POSIX_EXE)
add_test(
    NAME "EzCompilerEndToEnd_Hosted_test_posix_musl"
    COMMAND "${POSIX_EXE}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
set_tests_properties("EzCompilerEndToEnd_Hosted_test_posix_musl" PROPERTIES
    ENVIRONMENT_MODIFICATION "${HOSTED_RUN_ENV}"
    PASS_REGULAR_EXPRESSION "posix_musl ok"
    LABELS "E2E;Hosted;POSIX;Musl;EzCompiler"
)

# Negative E2E tests: freestanding rejects stdio, freestanding link rejects undefined libc symbols, syntax error fails
add_test(
    NAME "EzCompilerEndToEnd_Neg_FreestandingRejectsStdio"
    COMMAND ${CMAKE_COMMAND} -E env ${HOSTED_ENV} "${EZC_BIN}" -ffreestanding
        "${CMAKE_CURRENT_SOURCE_DIR}/c11_hosted/test_exec.c"
        -o "${CMAKE_CURRENT_BINARY_DIR}/neg_freestanding_stdio${HOSTED_OBJ_EXT}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
set_tests_properties("EzCompilerEndToEnd_Neg_FreestandingRejectsStdio" PROPERTIES
    WILL_FAIL TRUE
    LABELS "E2E;Negative;Freestanding;EzCompiler"
)

add_test(
    NAME "EzCompilerEndToEnd_Neg_FreestandingLinkRejectsUndefined"
    COMMAND ${CMAKE_COMMAND} -E env ${HOSTED_ENV} "${EZLD_BIN}" --freestanding --no-exception-rt
        "${CMAKE_CURRENT_BINARY_DIR}/test_exec_hosted${HOSTED_OBJ_EXT}"
        -o "${CMAKE_CURRENT_BINARY_DIR}/neg_freestanding_link${CMAKE_EXECUTABLE_SUFFIX}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
set_tests_properties("EzCompilerEndToEnd_Neg_FreestandingLinkRejectsUndefined" PROPERTIES
    WILL_FAIL TRUE
    LABELS "E2E;Negative;Freestanding;EzLinker"
)

add_test(
    NAME "EzCompilerEndToEnd_Neg_HostedSyntaxError"
    COMMAND ${CMAKE_COMMAND} -E env ${HOSTED_ENV} "${EZC_BIN}" -fhosted
        "${CMAKE_CURRENT_SOURCE_DIR}/c11_hosted/neg_syntax_error.c"
        -o "${CMAKE_CURRENT_BINARY_DIR}/neg_syntax_error${HOSTED_OBJ_EXT}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)
set_tests_properties("EzCompilerEndToEnd_Neg_HostedSyntaxError" PROPERTIES
    WILL_FAIL TRUE
    LABELS "E2E;Negative;Hosted;EzCompiler"
)

# Master CMake target for the end-to-end test suite
add_custom_target(EzCompilerEndToEndTests ALL
    DEPENDS ${ALL_E2E_OUTPUTS} elf_validator
)
