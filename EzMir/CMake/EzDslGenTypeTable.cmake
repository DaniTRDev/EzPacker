# cmake/EzDslGen.cmake

#[=======================================================================[.rst:
EzDslGenerateTypeTable
-----------------------------

Binds a .tyf type specification to a target library by executing ezdsl-gen
and attaching the generated MirTypeTable.h and MirTypeTable.cpp files.

Usage:
  EzDslGenerateTypeTable(
      TARGET <target_name>
      INPUT  <path_to_tyf_file>
      [OUTPUT_DIR <output_directory>]
  )
#]=======================================================================]
function(EzDslGenerateTypeTable)
    cmake_parse_arguments(PARSE_ARGV 0 EZDSL "" "TARGET;INPUT;OUTPUT_DIR" "")

    if(NOT EZDSL_TARGET)
        message(FATAL_ERROR "EzDslGenerateTypeTable: TARGET argument is required.")
    endif()

    if(NOT EZDSL_INPUT)
        message(FATAL_ERROR "EzDslGenerateTypeTable: INPUT argument is required.")
    endif()

    # Default output directory inside the binary tree
    if(NOT EZDSL_OUTPUT_DIR)
        set(EZDSL_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/Type")
    endif()

    set(GEN_HEADER "${EZDSL_OUTPUT_DIR}/MirTypeTable.h")
    set(GEN_SOURCE "${EZDSL_OUTPUT_DIR}/MirTypeTable.cpp")

    # Ensure output directory exists
    file(MAKE_DIRECTORY "${EZDSL_OUTPUT_DIR}")

    # Build custom command with dependency tracking
    add_custom_command(
        OUTPUT "${GEN_HEADER}" "${GEN_SOURCE}"
        COMMAND EzDsl-cli
                -i "${EZDSL_INPUT}"
                -o "${EZDSL_OUTPUT_DIR}"
                --emit-type-table
        DEPENDS EzDsl-cli "${EZDSL_INPUT}"
        COMMENT "[EzDSL] Synthesizing MirTypeTable from ${EZDSL_INPUT}"
        VERBATIM
    )

    # Attach generated files to the target
    target_sources(${EZDSL_TARGET} PRIVATE
        "${GEN_HEADER}"
        "${GEN_SOURCE}"
    )

    # Expose output directory to target's include path
    target_include_directories(${EZDSL_TARGET} PUBLIC
        "$<BUILD_INTERFACE:${EZDSL_OUTPUT_DIR}>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>"
    )

    target_sources(${EZDSL_TARGET} PRIVATE
        "${GEN_HEADER}"
        "${GEN_SOURCE}"
    )

    target_include_directories(${EZDSL_TARGET} PUBLIC
        "$<BUILD_INTERFACE:${EZDSL_OUTPUT_DIR}>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>"
    )
endfunction()