cmake_minimum_required(VERSION 3.27)
project(EzMusl C)

set(EZMUSL_UPSTREAM_DIR "${CMAKE_SOURCE_DIR}/third_party/musl")
set(EZMUSL_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(EZMUSL_GEN_INCLUDE_DIR "${EZMUSL_GEN_DIR}/include")
set(EZMUSL_GEN_INTERNAL_DIR "${EZMUSL_GEN_DIR}/internal")
set(EZMUSL_STAGE_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/staged_include")

if (NOT DEFINED EZMUSL_ARCH OR EZMUSL_ARCH STREQUAL "")
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "riscv64|RISCV64")
        set(EZMUSL_ARCH "riscv64")
    else ()
        set(EZMUSL_ARCH "x86_64")
    endif ()
endif ()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateMuslHeaders.cmake")

# Generate musl arch/internal headers using pure CMake (no sed/sh/awk required)
ezmusl_generate_alltypes(
    "${EZMUSL_UPSTREAM_DIR}/arch/${EZMUSL_ARCH}/bits/alltypes.h.in"
    "${EZMUSL_UPSTREAM_DIR}/include/alltypes.h.in"
    "${EZMUSL_GEN_INCLUDE_DIR}/bits/alltypes.h"
)
ezmusl_generate_syscall(
    "${EZMUSL_UPSTREAM_DIR}/arch/${EZMUSL_ARCH}/bits/syscall.h.in"
    "${EZMUSL_GEN_INCLUDE_DIR}/bits/syscall.h"
)
ezmusl_generate_version(
    "${EZMUSL_UPSTREAM_DIR}/VERSION"
    "${EZMUSL_GEN_INTERNAL_DIR}/version.h"
)

if (NOT DEFINED EZPACKER_TARGET_BIN_DIR OR EZPACKER_TARGET_BIN_DIR STREQUAL "")
    set(EZPACKER_TARGET_BIN_DIR "${CMAKE_BINARY_DIR}/bin")
endif ()

# Stage public musl headers into both build/bin/include/musl and build-local staged_include
ezmusl_stage_public_headers(
    "${EZMUSL_UPSTREAM_DIR}"
    "${EZMUSL_ARCH}"
    "${EZMUSL_GEN_INCLUDE_DIR}"
    "${EZMUSL_STAGE_INCLUDE_DIR}"
)
ezmusl_stage_public_headers(
    "${EZMUSL_UPSTREAM_DIR}"
    "${EZMUSL_ARCH}"
    "${EZMUSL_GEN_INCLUDE_DIR}"
    "${EZPACKER_TARGET_BIN_DIR}/include/musl"
)

file(MAKE_DIRECTORY "${EZPACKER_TARGET_BIN_DIR}/include/musl-compat")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/compat_include/" DESTINATION "${EZPACKER_TARGET_BIN_DIR}/include/musl-compat")

# Collect musl base C sources (src/*/*.c and src/malloc/mallocng/*.c)
file(GLOB EZMUSL_BASE_C "${EZMUSL_UPSTREAM_DIR}/src/*/*.c")
file(GLOB EZMUSL_MALLOCNG_C "${EZMUSL_UPSTREAM_DIR}/src/malloc/mallocng/*.c")
list(APPEND EZMUSL_BASE_C ${EZMUSL_MALLOCNG_C})

set(EZMUSL_SOURCES "")

if (WIN32)
    # Windows PE/COFF build: portable C musl sources + Win32 syscall/mman/CRT abstraction
    set(_win32_excluded
        "src/mman/mmap.c"
        "src/mman/munmap.c"
        "src/mman/mremap.c"
        "src/mman/madvise.c"
        "src/mman/mprotect.c"
        "src/thread/__set_thread_area.c"
        "src/thread/__unmapself.c"
        "src/thread/clone.c"
        "src/thread/syscall_cp.c"
        "src/process/vfork.c"
        "src/signal/restore.c"
        "src/signal/sigsetjmp.c"
    )

    # Collect x86_64 C math overrides when using GCC/Clang
    set(_arch_c_keys "")
    if (NOT MSVC AND EXISTS "${EZMUSL_UPSTREAM_DIR}/src/math/${EZMUSL_ARCH}")
        file(GLOB _math_arch_c "${EZMUSL_UPSTREAM_DIR}/src/math/${EZMUSL_ARCH}/*.c")
        foreach(_f IN LISTS _math_arch_c)
            file(RELATIVE_PATH _rel "${EZMUSL_UPSTREAM_DIR}" "${_f}")
            string(REPLACE "/${EZMUSL_ARCH}/" "/" _key "${_rel}")
            list(APPEND _arch_c_keys "${_key}")
            list(APPEND EZMUSL_SOURCES "${_f}")
        endforeach()
    endif ()

    foreach(_f IN LISTS EZMUSL_BASE_C)
        file(RELATIVE_PATH _rel "${EZMUSL_UPSTREAM_DIR}" "${_f}")
        if (NOT _rel IN_LIST _win32_excluded AND NOT _rel IN_LIST _arch_c_keys)
            list(APPEND EZMUSL_SOURCES "${_f}")
        endif ()
    endforeach()

    list(APPEND EZMUSL_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/win32/src/win32_syscall.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/win32/src/win32_mman.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/win32/src/win32_crt.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/win32/src/win32_stubs.c"
    )

    # Optional/overridable hooks keep COFF weak semantics; public API aliases are strong so ar indexes them
    set(_win32_weak_hook_files
        "${EZMUSL_UPSTREAM_DIR}/src/env/clearenv.c"
        "${EZMUSL_UPSTREAM_DIR}/src/env/putenv.c"
        "${EZMUSL_UPSTREAM_DIR}/src/env/unsetenv.c"
        "${EZMUSL_UPSTREAM_DIR}/src/env/__libc_start_main.c"
        "${EZMUSL_UPSTREAM_DIR}/src/exit/exit.c"
        "${EZMUSL_UPSTREAM_DIR}/src/exit/quick_exit.c"
        "${EZMUSL_UPSTREAM_DIR}/src/ldso/dladdr.c"
        "${EZMUSL_UPSTREAM_DIR}/src/ldso/dlerror.c"
        "${EZMUSL_UPSTREAM_DIR}/src/ldso/dlopen.c"
        "${EZMUSL_UPSTREAM_DIR}/src/ldso/dl_iterate_phdr.c"
        "${EZMUSL_UPSTREAM_DIR}/src/ldso/__dlsym.c"
        "${EZMUSL_UPSTREAM_DIR}/src/linux/membarrier.c"
        "${EZMUSL_UPSTREAM_DIR}/src/locale/dcngettext.c"
        "${EZMUSL_UPSTREAM_DIR}/src/locale/__lctrans.c"
        "${EZMUSL_UPSTREAM_DIR}/src/malloc/calloc.c"
        "${EZMUSL_UPSTREAM_DIR}/src/malloc/lite_malloc.c"
        "${EZMUSL_UPSTREAM_DIR}/src/math/tgamma.c"
        "${EZMUSL_UPSTREAM_DIR}/src/process/fork.c"
        "${EZMUSL_UPSTREAM_DIR}/src/stdio/fflush.c"
        "${EZMUSL_UPSTREAM_DIR}/src/stdio/__stdio_close.c"
        "${EZMUSL_UPSTREAM_DIR}/src/stdio/__stdio_exit.c"
        "${EZMUSL_UPSTREAM_DIR}/src/thread/pthread_create.c"
        "${EZMUSL_UPSTREAM_DIR}/src/thread/pthread_key_create.c"
        "${EZMUSL_UPSTREAM_DIR}/src/thread/synccall.c"
        "${EZMUSL_UPSTREAM_DIR}/src/time/timer_create.c"
        "${EZMUSL_UPSTREAM_DIR}/src/unistd/close.c"
    )
    set_source_files_properties(${_win32_weak_hook_files} PROPERTIES
        COMPILE_DEFINITIONS "EZ_MUSL_WEAK_HOOKS=1"
    )
else ()
    # Native Linux / ELF64 build: upstream musl base + arch overrides (.c, .s, .S)
    enable_language(ASM)

    file(GLOB EZMUSL_ARCH_SRCS
        "${EZMUSL_UPSTREAM_DIR}/src/*/${EZMUSL_ARCH}/*.c"
        "${EZMUSL_UPSTREAM_DIR}/src/*/${EZMUSL_ARCH}/*.s"
        "${EZMUSL_UPSTREAM_DIR}/src/*/${EZMUSL_ARCH}/*.S"
    )

    set(_arch_override_keys "")
    foreach(_f IN LISTS EZMUSL_ARCH_SRCS)
        get_filename_component(_dir "${_f}" DIRECTORY)
        get_filename_component(_name_we "${_f}" NAME_WE)
        get_filename_component(_parent_dir "${_dir}" DIRECTORY)
        file(RELATIVE_PATH _rel_parent "${EZMUSL_UPSTREAM_DIR}" "${_parent_dir}")
        list(APPEND _arch_override_keys "${_rel_parent}/${_name_we}")
        list(APPEND EZMUSL_SOURCES "${_f}")
    endforeach()

    foreach(_f IN LISTS EZMUSL_BASE_C)
        get_filename_component(_dir "${_f}" DIRECTORY)
        get_filename_component(_name_we "${_f}" NAME_WE)
        file(RELATIVE_PATH _rel_dir "${EZMUSL_UPSTREAM_DIR}" "${_dir}")
        if (NOT "${_rel_dir}/${_name_we}" IN_LIST _arch_override_keys)
            list(APPEND EZMUSL_SOURCES "${_f}")
        endif ()
    endforeach()
endif ()

# Configure static musl library via EzCMK
EzCMK_ConfigureLib(NAME "EzMusl"
    BUILD_TYPE STATIC
    FILE_LIST ${EZMUSL_SOURCES}
)

if (DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY AND NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY STREQUAL "")
    set(EZMUSL_ARCHIVE_DIR "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
else ()
    set(EZMUSL_ARCHIVE_DIR "${CMAKE_BINARY_DIR}/lib")
endif ()

set_target_properties(EzMusl PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${EZMUSL_ARCHIVE_DIR}"
    C_STANDARD 99
    C_STANDARD_REQUIRED OFF
)

if (WIN32)
    target_include_directories(EzMusl PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/win32/include"
    )
endif ()

target_include_directories(EzMusl PRIVATE
    "${EZMUSL_UPSTREAM_DIR}/arch/${EZMUSL_ARCH}"
    "${EZMUSL_UPSTREAM_DIR}/arch/generic"
    "${EZMUSL_GEN_INTERNAL_DIR}"
    "${EZMUSL_UPSTREAM_DIR}/src/include"
    "${EZMUSL_UPSTREAM_DIR}/src/internal"
    "${EZMUSL_GEN_INCLUDE_DIR}"
    "${EZMUSL_UPSTREAM_DIR}/include"
)

target_compile_definitions(EzMusl PRIVATE
    _XOPEN_SOURCE=700
)

if (MSVC)
    target_compile_options(EzMusl PRIVATE
        /O2 /GS- /W0
    )
else ()
    target_compile_options(EzMusl PRIVATE
        -std=c99
        -ffreestanding
        -nostdinc
        -Os
        -fno-stack-protector
        -fno-builtin
        -Wno-parentheses
        -Wno-unused-value
        -Wno-dangling-else
        -Wno-unknown-pragmas
        -Wno-attributes
        -Wno-int-conversion
        -Wno-incompatible-pointer-types
        -Wno-overflow
    )
    if (CMAKE_C_COMPILER_ID MATCHES "Clang")
        target_compile_options(EzMusl PRIVATE
            -Wno-shift-op-parentheses
            -Wno-bitwise-op-parentheses
            -Wno-logical-op-parentheses
        )
    endif ()
    if (WIN32)
        # Prevent MinGW GCC from emitting ___chkstk_ms / libgcc stack-probe calls so
        # libEzMusl.a links cleanly under both MSVC link.exe / lld-link and MinGW ld.
        target_compile_options(EzMusl PRIVATE -mno-stack-arg-probe)
    endif ()
endif ()

# On Linux/ELF, also build standalone crt1.o into build/lib/crt1.o
if (NOT WIN32)
    add_library(EzMusl_crt1_obj OBJECT "${EZMUSL_UPSTREAM_DIR}/crt/crt1.c")
    target_include_directories(EzMusl_crt1_obj PRIVATE
        "${EZMUSL_UPSTREAM_DIR}/arch/${EZMUSL_ARCH}"
        "${EZMUSL_UPSTREAM_DIR}/arch/generic"
        "${EZMUSL_GEN_INTERNAL_DIR}"
        "${EZMUSL_UPSTREAM_DIR}/src/include"
        "${EZMUSL_UPSTREAM_DIR}/src/internal"
        "${EZMUSL_GEN_INCLUDE_DIR}"
        "${EZMUSL_UPSTREAM_DIR}/include"
    )
    target_compile_definitions(EzMusl_crt1_obj PRIVATE _XOPEN_SOURCE=700 CRT)
    target_compile_options(EzMusl_crt1_obj PRIVATE
        -std=c99 -ffreestanding -nostdinc -Os -fno-stack-protector -fno-builtin
    )
    add_custom_command(
        OUTPUT "${EZMUSL_ARCHIVE_DIR}/crt1.o"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${EZMUSL_ARCHIVE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_OBJECTS:EzMusl_crt1_obj>" "${EZMUSL_ARCHIVE_DIR}/crt1.o"
        DEPENDS EzMusl_crt1_obj $<TARGET_OBJECTS:EzMusl_crt1_obj>
        COMMENT "[EzMusl] Staging crt1.o to ${EZMUSL_ARCHIVE_DIR}/crt1.o"
        VERBATIM
    )
    add_custom_target(EzMusl_crt1 ALL DEPENDS "${EZMUSL_ARCHIVE_DIR}/crt1.o")
    add_dependencies(EzMusl EzMusl_crt1)
endif ()

# Target to keep staged headers in sync on incremental rebuilds
add_custom_target(EzMusl_StageHeaders ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${EZPACKER_TARGET_BIN_DIR}/include/musl"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${EZMUSL_STAGE_INCLUDE_DIR}" "${EZPACKER_TARGET_BIN_DIR}/include/musl"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${EZPACKER_TARGET_BIN_DIR}/include/musl-compat"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/compat_include" "${EZPACKER_TARGET_BIN_DIR}/include/musl-compat"
    COMMENT "[EzMusl] Staging musl headers to ${EZPACKER_TARGET_BIN_DIR}/include/musl"
    VERBATIM
)
add_dependencies(EzMusl EzMusl_StageHeaders)
