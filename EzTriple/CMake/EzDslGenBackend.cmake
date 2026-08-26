#[=======================================================================[.rst:
EzDslGenBackend
---------------

Provides CMake functions to synthesize EzTriple backend code from EzDSL definition files.

.. command:: EzDslGenTarget

  Synthesizes C++ target architecture definitions from EzDSL files::

    EzDslGenTarget(<TargetName>
      OUTPUT_DIR <dir>
      TDF <tdf_file>
      IDF <idf_file>
      LAD <lad_file>
      LRD <lrd_file>
      ISF <isf_file>
      CCDF <ccdf_file>
      TYF <tyf_file>
      IRDF <irdf_file>
    )

#]=======================================================================]

function(EzDslGenTarget TARGET_NAME)
    cmake_parse_arguments(ARG "" "OUTPUT_DIR;TDF;IDF;LAD;LRD;ISF;CCDF;TYF;IRDF" "" ${ARGN})

    if(NOT ARG_OUTPUT_DIR)
        set(ARG_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/${TARGET_NAME}")
    endif()

    set(GENERATED_HEADERS
        "${ARG_OUTPUT_DIR}/${TARGET_NAME}InstructionDefs.h"
        "${ARG_OUTPUT_DIR}/${TARGET_NAME}RegisterBanks.h"
        "${ARG_OUTPUT_DIR}/${TARGET_NAME}TypeLayout.h"
        "${ARG_OUTPUT_DIR}/${TARGET_NAME}LegalizerActionTable.h"
        "${ARG_OUTPUT_DIR}/${TARGET_NAME}LegalizeRules.h"
        "${ARG_OUTPUT_DIR}/${TARGET_NAME}ISelTable.h"
        "${ARG_OUTPUT_DIR}/${TARGET_NAME}CallingConventions.h"
        "${ARG_OUTPUT_DIR}/${TARGET_NAME}TargetDesc.h"
    )

    set(GENERATED_SOURCES
        "${ARG_OUTPUT_DIR}/${TARGET_NAME}InstructionDefs.cpp"
        "${ARG_OUTPUT_DIR}/${TARGET_NAME}BinaryEncoder.cpp"
        "${ARG_OUTPUT_DIR}/${TARGET_NAME}RegisterBanks.cpp"
        "${ARG_OUTPUT_DIR}/${TARGET_NAME}TypeLayout.cpp"
        "${ARG_OUTPUT_DIR}/${TARGET_NAME}LegalizerActionTable.cpp"
        "${ARG_OUTPUT_DIR}/${TARGET_NAME}LegalizeRules.cpp"
        "${ARG_OUTPUT_DIR}/${TARGET_NAME}ISelTable.cpp"
        "${ARG_OUTPUT_DIR}/${TARGET_NAME}CallingConventions.cpp"
        "${ARG_OUTPUT_DIR}/${TARGET_NAME}TargetDesc.cpp"
    )

    set(CLI_ARGS --target ${TARGET_NAME} -o "${ARG_OUTPUT_DIR}" --emit-all)

    if(ARG_TDF)
        list(APPEND CLI_ARGS --tdf "${ARG_TDF}")
    endif()
    if(ARG_IDF)
        list(APPEND CLI_ARGS --idf "${ARG_IDF}")
    endif()
    if(ARG_LAD)
        list(APPEND CLI_ARGS --lad "${ARG_LAD}")
    endif()
    if(ARG_LRD)
        list(APPEND CLI_ARGS --lrd "${ARG_LRD}")
    endif()
    if(ARG_ISF)
        list(APPEND CLI_ARGS --isf "${ARG_ISF}")
    endif()
    if(ARG_CCDF)
        list(APPEND CLI_ARGS --ccdf "${ARG_CCDF}")
    endif()
    if(ARG_TYF)
        list(APPEND CLI_ARGS --tyf "${ARG_TYF}")
    endif()
    if(ARG_IRDF)
        list(APPEND CLI_ARGS --irdf "${ARG_IRDF}")
    endif()

    add_custom_command(
        OUTPUT ${GENERATED_HEADERS} ${GENERATED_SOURCES}
        COMMAND EzDsl-cli ${CLI_ARGS}
        DEPENDS EzDsl-cli ${ARG_TDF} ${ARG_IDF} ${ARG_LAD} ${ARG_LRD} ${ARG_ISF} ${ARG_CCDF} ${ARG_TYF} ${ARG_IRDF}
        COMMENT "Synthesizing EzTriple target backend for ${TARGET_NAME} using EzDsl-cli"
        VERBATIM
    )

    set(${TARGET_NAME}_GENERATED_HEADERS ${GENERATED_HEADERS} PARENT_SCOPE)
    set(${TARGET_NAME}_GENERATED_SOURCES ${GENERATED_SOURCES} PARENT_SCOPE)
endfunction()
