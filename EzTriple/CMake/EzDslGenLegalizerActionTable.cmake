# EzTriple/CMake/EzDslGenLegalizerActionTable.cmake

#[=======================================================================[.rst:
EzDslGenLegalizerActionTable
----------------------------

Binds an .lad legalizer action specification file (and optional companion .lrd rules)
to a target library by executing EzDslCli and generating <Target>LegalizerActionTable.h/.cpp
and <Target>LegalizerRules.h/.cpp.

Usage:
  EzDslGenLegalizerActionTable(
      TARGET <target_name>
      INPUT  <path_to_lad_file>
      [RULES <path_to_lrd_file>]
      TARGET_NAME <target_architecture_name>
      [OUTPUT_DIR <output_directory>]
  )
#]=======================================================================]
function(EzDslGenLegalizerActionTable)
    cmake_parse_arguments(PARSE_ARGV 0 EZDSL "" "TARGET;INPUT;RULES;TARGET_NAME;OUTPUT_DIR" "")

    if(NOT EZDSL_TARGET)
        message(FATAL_ERROR "EzDslGenLegalizerActionTable: TARGET argument is required.")
    endif()

    if(NOT EZDSL_INPUT)
        message(FATAL_ERROR "EzDslGenLegalizerActionTable: INPUT argument is required.")
    endif()

    if(NOT EZDSL_TARGET_NAME)
        set(EZDSL_TARGET_NAME "Target")
    endif()

    if(NOT EZDSL_OUTPUT_DIR)
        set(EZDSL_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/${EZDSL_TARGET_NAME}")
    endif()

    set(GEN_HEADER "${EZDSL_OUTPUT_DIR}/${EZDSL_TARGET_NAME}LegalizerActionTable.h")
    set(GEN_SOURCE "${EZDSL_OUTPUT_DIR}/${EZDSL_TARGET_NAME}LegalizerActionTable.cpp")
    set(GEN_OUTPUTS "${GEN_HEADER}" "${GEN_SOURCE}")
    set(GEN_DEPENDS EzDslCli "${EZDSL_INPUT}")

    set(RULES_ARG "")
    if(EZDSL_RULES)
        set(GEN_RULES_HEADER "${EZDSL_OUTPUT_DIR}/${EZDSL_TARGET_NAME}LegalizerRules.h")
        set(GEN_RULES_SOURCE "${EZDSL_OUTPUT_DIR}/${EZDSL_TARGET_NAME}LegalizerRules.cpp")
        list(APPEND GEN_OUTPUTS "${GEN_RULES_HEADER}" "${GEN_RULES_SOURCE}")
        list(APPEND GEN_DEPENDS "${EZDSL_RULES}")
        set(RULES_ARG --rules "${EZDSL_RULES}")
    endif()

    file(MAKE_DIRECTORY "${EZDSL_OUTPUT_DIR}")

    set(ENV_WRAPPER "")
    if(WIN32)
        get_filename_component(COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
        set(ENV_WRAPPER ${CMAKE_COMMAND} -E env "PATH=${CMAKE_BINARY_DIR}/bin\;${COMPILER_DIR}\;C:/Windows/system32\;C:/Windows" --)
    endif()

    add_custom_command(
        OUTPUT ${GEN_OUTPUTS}
        COMMAND ${ENV_WRAPPER} $<TARGET_FILE:EzDslCli>
                -i "${EZDSL_INPUT}"
                ${RULES_ARG}
                -o "${EZDSL_OUTPUT_DIR}"
                --emit-legalizer
                --target "${EZDSL_TARGET_NAME}"
        DEPENDS ${GEN_DEPENDS}
        COMMENT "[EzDSL] Synthesizing ${EZDSL_TARGET_NAME}LegalizerActionTable from ${EZDSL_INPUT}"
        VERBATIM
    )

    target_sources(${EZDSL_TARGET} PRIVATE
        ${GEN_OUTPUTS}
    )

    target_include_directories(${EZDSL_TARGET} PUBLIC
        "$<BUILD_INTERFACE:${EZDSL_OUTPUT_DIR}>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>"
    )
endfunction()
