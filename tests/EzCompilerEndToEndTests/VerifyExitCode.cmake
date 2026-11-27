cmake_minimum_required(VERSION 3.27)

if(NOT DEFINED TEST_EXE OR NOT DEFINED EXPECTED_EXIT_CODE)
    message(FATAL_ERROR "TEST_EXE and EXPECTED_EXIT_CODE must be defined")
endif()

execute_process(
    COMMAND "${TEST_EXE}"
    RESULT_VARIABLE _rc
    OUTPUT_VARIABLE _out
    ERROR_VARIABLE _err
)

if(NOT _rc EQUAL EXPECTED_EXIT_CODE)
    message(FATAL_ERROR "Expected exit code ${EXPECTED_EXIT_CODE}, got ${_rc}\nstdout:\n${_out}\nstderr:\n${_err}")
endif()

if(DEFINED EXPECTED_OUTPUT_SUBSTR AND NOT EXPECTED_OUTPUT_SUBSTR STREQUAL "")
    string(FIND "${_out}" "${EXPECTED_OUTPUT_SUBSTR}" _pos)
    if(_pos EQUAL -1)
        message(FATAL_ERROR "Expected stdout to contain '${EXPECTED_OUTPUT_SUBSTR}', got:\n${_out}")
    endif()
endif()
