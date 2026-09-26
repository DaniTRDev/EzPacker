# CopyStandardLibraries.cmake
# Automatically discovers standard compiler runtime libraries (e.g., libstdc++, libgcc_s, libwinpthread)
# and copies them to the runtime output directory (bin) to ensure executables and tests run
# on systems without the compiler toolchain installed.

option(EZPACKER_COPY_STANDARD_LIBRARIES "Copy compiler standard libraries to the project's bin output directory" ON)

if (NOT EZPACKER_COPY_STANDARD_LIBRARIES)
    return()
endif ()

if (DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY AND NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY STREQUAL "")
    set(EZPACKER_TARGET_BIN_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
else ()
    set(EZPACKER_TARGET_BIN_DIR "${CMAKE_BINARY_DIR}/bin")
endif ()

set(EZPACKER_SEARCH_DIRS "")

# 1. Compiler binary directory
if (CMAKE_CXX_COMPILER)
    get_filename_component(_CXX_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    list(APPEND EZPACKER_SEARCH_DIRS "${_CXX_DIR}")
endif ()
if (CMAKE_C_COMPILER)
    get_filename_component(_C_DIR "${CMAKE_C_COMPILER}" DIRECTORY)
    list(APPEND EZPACKER_SEARCH_DIRS "${_C_DIR}")
endif ()

# 2. Query compiler for libstdc++ static/import library location to derive GCC sysroot/lib paths
if (CMAKE_CXX_COMPILER)
    execute_process(
        COMMAND "${CMAKE_CXX_COMPILER}" -print-file-name=libstdc++.a
        OUTPUT_VARIABLE _LIBSTDCXX_A
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if (_LIBSTDCXX_A AND EXISTS "${_LIBSTDCXX_A}")
        get_filename_component(_LIBSTDCXX_DIR "${_LIBSTDCXX_A}" DIRECTORY)
        list(APPEND EZPACKER_SEARCH_DIRS
            "${_LIBSTDCXX_DIR}"
            "${_LIBSTDCXX_DIR}/../bin"
            "${_LIBSTDCXX_DIR}/../../bin"
            "${_LIBSTDCXX_DIR}/../../../bin"
            "${_LIBSTDCXX_DIR}/../../../../bin"
        )
    endif ()
endif ()

# 3. MinGW / toolchain sysroot directories
if (CMAKE_SYSROOT)
    list(APPEND EZPACKER_SEARCH_DIRS
        "${CMAKE_SYSROOT}/bin"
        "${CMAKE_SYSROOT}/lib"
    )
endif ()

if (WIN32)
    # Target search patterns for Windows (MinGW GCC, Clang, etc.)
    set(EZPACKER_STD_PATTERNS
        "libstdc++*.dll"
        "libgcc_s*.dll"
        "libwinpthread*.dll"
        "libatomic*.dll"
        "libc++*.dll"
        "libunwind*.dll"
    )
else ()
    # Non-Windows shared objects
    set(EZPACKER_STD_PATTERNS
        "libstdc++.so*"
        "libgcc_s.so*"
    )
endif ()

set(EZPACKER_FOUND_STD_LIBS "")

foreach(_DIR IN LISTS EZPACKER_SEARCH_DIRS)
    cmake_path(NORMAL_PATH _DIR)
    if (EXISTS "${_DIR}")
        foreach(_PAT IN LISTS EZPACKER_STD_PATTERNS)
            file(GLOB _MATCHES "${_DIR}/${_PAT}")
            list(APPEND EZPACKER_FOUND_STD_LIBS ${_MATCHES})
        endforeach()
    endif ()
endforeach()

# Fallback: if libstdc++-6.dll wasn't found in compiler dirs, search PATH entries
if (WIN32 AND NOT EZPACKER_FOUND_STD_LIBS)
    file(TO_CMAKE_PATH "$ENV{PATH}" _ENV_PATH)
    foreach(_DIR IN LISTS _ENV_PATH)
        if (EXISTS "${_DIR}")
            file(GLOB _MATCHES "${_DIR}/libstdc++*.dll")
            if (_MATCHES)
                list(APPEND EZPACKER_SEARCH_DIRS "${_DIR}")
                foreach(_PAT IN LISTS EZPACKER_STD_PATTERNS)
                    file(GLOB _M "${_DIR}/${_PAT}")
                    list(APPEND EZPACKER_FOUND_STD_LIBS ${_M})
                endforeach()
                break()
            endif ()
        endif ()
    endforeach()
endif ()

# Support MSVC runtime DLLs if compiling with MSVC
if (MSVC)
    include(InstallRequiredSystemLibraries)
    if (CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
        list(APPEND EZPACKER_FOUND_STD_LIBS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS})
    endif ()
endif ()

if (EZPACKER_FOUND_STD_LIBS)
    list(REMOVE_DUPLICATES EZPACKER_FOUND_STD_LIBS)
    message(STATUS "[EzPacker] Discovered standard runtime libraries to copy:")
    foreach(_LIB IN LISTS EZPACKER_FOUND_STD_LIBS)
        message(STATUS "  - ${_LIB}")
    endforeach()

    # Ensure destination directory exists at configure time
    file(MAKE_DIRECTORY "${EZPACKER_TARGET_BIN_DIR}")

    # Copy files during CMake configuration step
    foreach(_LIB IN LISTS EZPACKER_FOUND_STD_LIBS)
        get_filename_component(_LIB_NAME "${_LIB}" NAME)
        set(_DEST_FILE "${EZPACKER_TARGET_BIN_DIR}/${_LIB_NAME}")
        if (NOT EXISTS "${_DEST_FILE}" OR "${_LIB}" IS_NEWER_THAN "${_DEST_FILE}")
            file(COPY "${_LIB}" DESTINATION "${EZPACKER_TARGET_BIN_DIR}")
        endif ()
    endforeach()

    # Create build-time target to re-copy on rebuilds or clean builds
    if (CMAKE_CONFIGURATION_TYPES)
        add_custom_target(EzPacker_CopyStandardLibraries ALL
            COMMAND ${CMAKE_COMMAND} -E make_directory "${EZPACKER_TARGET_BIN_DIR}/$<CONFIG>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EZPACKER_FOUND_STD_LIBS} "${EZPACKER_TARGET_BIN_DIR}/$<CONFIG>"
            COMMENT "[EzPacker] Copying standard libraries to ${EZPACKER_TARGET_BIN_DIR}/$<CONFIG>"
            VERBATIM
        )
    else ()
        add_custom_target(EzPacker_CopyStandardLibraries ALL
            COMMAND ${CMAKE_COMMAND} -E make_directory "${EZPACKER_TARGET_BIN_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${EZPACKER_FOUND_STD_LIBS} "${EZPACKER_TARGET_BIN_DIR}"
            COMMENT "[EzPacker] Copying standard libraries to ${EZPACKER_TARGET_BIN_DIR}"
            VERBATIM
        )
    endif ()
else ()
    message(STATUS "[EzPacker] No compiler standard runtime libraries required or found to copy.")
endif ()
