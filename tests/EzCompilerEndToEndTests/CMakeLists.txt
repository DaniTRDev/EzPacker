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

# Master CMake target for the end-to-end test suite
add_custom_target(EzCompilerEndToEndTests ALL
    DEPENDS ${ALL_E2E_OUTPUTS} elf_validator
)
