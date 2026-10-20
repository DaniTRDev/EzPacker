set(EZCOMPILER_LIB_FILES_LIST
        "include/EzCompilerCommon.h"
        "include/CommandLineOptions.h"
        "include/DriverContext.h"
        "include/TargetResolver.h"
        "include/TargetTriple.h"
        "include/FrontendAdapter.h"
        "include/CompilationPipeline.h"
        "include/EmissionEngine.h"
        "include/PipelineTelemetry.h"
        "include/CFrontend/ChibiccWrapper.h"
        "include/CFrontend/CFrontendTypeMapper.h"
        "include/CFrontend/CFrontendFunctionLowerer.h"
        "include/CFrontend/CFrontendAdapter.h"

        "src/CommandLineOptions.cpp"
        "src/DriverContext.cpp"
        "src/TargetResolver.cpp"
        "src/TargetTriple.cpp"
        "src/FrontendAdapter.cpp"
        "src/CompilationPipeline.cpp"
        "src/EmissionEngine.cpp"
        "src/PipelineTelemetry.cpp"
        "src/CFrontend/ChibiccWrapper.cpp"
        "src/CFrontend/CFrontendTypeMapper.cpp"
        "src/CFrontend/CFrontendFunctionLowerer.cpp"
        "src/CFrontend/CFrontendAdapter.cpp"
)

EzCMK_ConfigureLib(NAME "EzCompilerLib"
        FILE_LIST ${EZCOMPILER_LIB_FILES_LIST}
        PRECOMPILED_HEADER "include/EzCompilerCommon.h"
        LINKED_LIBRARIES "EzTriple;EzMir;EzCore;EzCodeEmitter;argparse;chibicc_frontend"
        INCLUDED_DIRS "include;../EzCodeEmitter/include;../EzTriple/include;../EzMir/include;../EzCore/include;${chibicc_SOURCE_DIR};${chibicc_SOURCE_DIR}/shims"
)

EzCMK_AddExecutable(NAME "ezc"
        FILE_LIST "src/Main.cpp"
        PRECOMPILED_HEADER "include/EzCompilerCommon.h"
        LINKED_LIBRARIES "EzCompilerLib;EzTargetsX86_64Registration;EzTargetsRiscV64Registration"
        INCLUDED_DIRS "include;${CMAKE_SOURCE_DIR}/EzTargets/X86_64/Registration/include;${CMAKE_SOURCE_DIR}/EzTargets/RiscV64/Registration/include"
)

target_compile_definitions(EzCompilerLib PRIVATE
    EZPACKER_FREESTANDING_INCLUDE_DIR="${CMAKE_SOURCE_DIR}/include/freestanding"
)

if (NOT DEFINED EZPACKER_TARGET_BIN_DIR OR EZPACKER_TARGET_BIN_DIR STREQUAL "")
    set(EZPACKER_TARGET_BIN_DIR "${CMAKE_BINARY_DIR}/bin")
endif ()

file(MAKE_DIRECTORY "${EZPACKER_TARGET_BIN_DIR}/include/freestanding")
file(COPY "${CMAKE_SOURCE_DIR}/include/freestanding/" DESTINATION "${EZPACKER_TARGET_BIN_DIR}/include/freestanding")

add_custom_target(EzPacker_CopyFreestandingHeaders ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${EZPACKER_TARGET_BIN_DIR}/include/freestanding"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/include/freestanding" "${EZPACKER_TARGET_BIN_DIR}/include/freestanding"
    COMMENT "[EzPacker] Copying freestanding headers to ${EZPACKER_TARGET_BIN_DIR}/include/freestanding"
    VERBATIM
)

add_dependencies(EzCompilerLib EzPacker_CopyFreestandingHeaders)
add_dependencies(ezc EzPacker_CopyFreestandingHeaders)

