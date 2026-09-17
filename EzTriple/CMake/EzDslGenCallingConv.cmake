# EzTriple/CMake/EzDslGenCallingConv.cmake

#[=======================================================================[.rst:
EzDslGenCallingConv
-------------------

Binds an .ezcc / .ccd calling convention specification file to a target library
by executing EzDslCli and generating <Target>CallingConvDesc.h and .cpp.

Usage:
  EzDslGenCallingConv(
      TARGET <target_name>
      INPUT  <path_to_ezcc_file>
      TARGET_NAME <target_architecture_name>
      [OUTPUT_DIR <output_directory>]
  )
#]=======================================================================]
function(EzDslGenCallingConv)
    cmake_parse_arguments(PARSE_ARGV 0 EZDSL "" "TARGET;INPUT;TARGET_NAME;OUTPUT_DIR" "")

    if(NOT EZDSL_TARGET)
        message(FATAL_ERROR "EzDslGenCallingConv: TARGET argument is required.")
    endif()

    if(NOT EZDSL_INPUT)
        message(FATAL_ERROR "EzDslGenCallingConv: INPUT argument is required.")
    endif()

    if(NOT EZDSL_TARGET_NAME)
        set(EZDSL_TARGET_NAME "Target")
    endif()

    if(NOT EZDSL_OUTPUT_DIR)
        set(EZDSL_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/${EZDSL_TARGET_NAME}")
    endif()

    set(GEN_HEADER "${EZDSL_OUTPUT_DIR}/${EZDSL_TARGET_NAME}CallingConvDesc.h")
    set(GEN_SOURCE "${EZDSL_OUTPUT_DIR}/${EZDSL_TARGET_NAME}CallingConvDesc.cpp")

    file(MAKE_DIRECTORY "${EZDSL_OUTPUT_DIR}")

    set(ENV_WRAPPER "")
    if(WIN32)
        get_filename_component(COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
        set(ENV_WRAPPER ${CMAKE_COMMAND} -E env "PATH=${CMAKE_BINARY_DIR}/bin\;${COMPILER_DIR}\;C:/Windows/system32\;C:/Windows" --)
    endif()

    add_custom_command(
        OUTPUT "${GEN_HEADER}" "${GEN_SOURCE}"
        COMMAND ${ENV_WRAPPER} $<TARGET_FILE:EzDslCli>
                -i "${EZDSL_INPUT}"
                -o "${EZDSL_OUTPUT_DIR}"
                --emit-calling-conv
                --target "${EZDSL_TARGET_NAME}"
        DEPENDS EzDslCli "${EZDSL_INPUT}"
        COMMENT "[EzDSL] Synthesizing ${EZDSL_TARGET_NAME}CallingConvDesc from ${EZDSL_INPUT}"
        VERBATIM
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
