# cmake/EzDslGen.cmake

#[=======================================================================[.rst:
EzDslGenMirInstructions
---------------------------

Binds an .irdf IR instruction specification file to a target library
by executing EzDslCli and generating MirInstructionSetDefs.h.

Usage:
  EzDslGenMirInstructions(
      TARGET <target_name>
      INPUT  <path_to_irdf_file>
      [OUTPUT_DIR <output_directory>]
  )
#]=======================================================================]
function(EzDslGenMirInstructions)
    cmake_parse_arguments(PARSE_ARGV 0 EZDSL "" "TARGET;INPUT;OUTPUT_DIR" "")

    if(NOT EZDSL_TARGET)
        message(FATAL_ERROR "EzDslGenerateIrInstructions: TARGET argument is required.")
    endif()

    if(NOT EZDSL_INPUT)
        message(FATAL_ERROR "EzDslGenerateIrInstructions: INPUT argument is required.")
    endif()

    # Default output directory inside the binary tree
    if(NOT EZDSL_OUTPUT_DIR)
        set(EZDSL_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/Instruction")
    endif()

    set(GEN_HEADER "${EZDSL_OUTPUT_DIR}/MirInstructionSetDefs.h")

    # Ensure output directory exists
    file(MAKE_DIRECTORY "${EZDSL_OUTPUT_DIR}")

    set(ENV_WRAPPER "")
    if(WIN32)
        get_filename_component(COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
        set(ENV_WRAPPER ${CMAKE_COMMAND} -E env "PATH=${CMAKE_BINARY_DIR}/bin\;${COMPILER_DIR}\;C:/Windows/system32\;C:/Windows" --)
    endif()

    # Build custom command with dependency tracking
    add_custom_command(
        OUTPUT "${GEN_HEADER}"
        COMMAND ${ENV_WRAPPER} $<TARGET_FILE:EzDslCli>
                -i "${EZDSL_INPUT}"
                -o "${EZDSL_OUTPUT_DIR}"
                --emit-instructions
        DEPENDS EzDslCli "${EZDSL_INPUT}"
        COMMENT "[EzDSL] Synthesizing MirInstructionSetDefs.h from ${EZDSL_INPUT}"
        VERBATIM
    )

    # Attach generated header to the target
    target_sources(${EZDSL_TARGET} PRIVATE
        "${GEN_HEADER}"
    )

    # Expose output directory to target's include path
    target_include_directories(${EZDSL_TARGET} PUBLIC
        "$<BUILD_INTERFACE:${EZDSL_OUTPUT_DIR}>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>"
    )
endfunction()