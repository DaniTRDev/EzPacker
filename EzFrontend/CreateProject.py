import os
import sys


def create_cmake_library_template(project_name):
    # Create directory structure
    directories = [
        f"{project_name}/include",
        f"{project_name}/src",
        f"{project_name}/tests"
    ]

    for directory in directories:
        os.makedirs(directory, exist_ok=True)

    # Create common header files
    common_header_content = f"""#ifndef {project_name.upper()}_COMMON_H
#define {project_name.upper()}_COMMON_H
// {project_name} Precompiled Header
#include <memory>
#include <vector>
#include <string>
#include <cstdint>

#endif // {project_name.upper()}_COMMON_H
"""
    with open(f"{project_name}/include/{project_name}Common.h", "w") as f:
        f.write(common_header_content)

    main_header_content = f"""#ifndef {project_name.upper()}_H
#define {project_name.upper()}_H

#include "{project_name}Common.h"

#endif // {project_name.upper()}_H
"""
    with open(f"{project_name}/include/{project_name}.h", "w") as f:
        f.write(main_header_content)

    # Create CMakeLists.txt
    cmake_content = f"""# {project_name} CMake build configuration

function(GetLibraryBuildType out)
    if (EZPACKER_BUILD_STATIC)
        set(${{out}} "STATIC" PARENT_SCOPE)
    else ()
        set(${{out}} "SHARED" PARENT_SCOPE)
    endif ()
endfunction()

function(Build{project_name})
    GetLibraryBuildType({project_name.upper()}_BUILD_TYPE)
    message(STATUS "Building {project_name} as ${{{project_name.upper()}_BUILD_TYPE}}")

    add_library("{project_name}" ${{{project_name.upper()}_BUILD_TYPE}}
            "include/{project_name}.h"
            "include/{project_name}Common.h"
    )

    # Set precompiled header
    target_precompile_headers({project_name} PUBLIC "${{CMAKE_CURRENT_SOURCE_DIR}}/include/{project_name}Common.h")

    # Add include directories and link dependencies
    target_link_libraries({project_name} PUBLIC EzLogger gtest gtest_main)
    target_include_directories({project_name} PUBLIC "${{CMAKE_CURRENT_SOURCE_DIR}}/include" ${{gtest_build_include_dirs}})
    set_target_properties({project_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${{CMAKE_BINARY_DIR}}/bin")
endfunction()

Build{project_name}()

# Add tests
if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for {project_name}")

    #EzPacker_AddTest(
    #       "T_{project_name}Test"
    #       ""
    #       "{project_name}"
    #       ""
    #       "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/Test{project_name}.cpp"
    #)
endif()
"""
    with open(f"{project_name}/CMakeLists.txt", "w") as f:
        f.write(cmake_content)

    print(f"Successfully created {project_name} project structure and CMake configuration.")


if __name__ == "__main__":

    input_name: str = ""
    if len(sys.argv) != 2:
        input_name = input("Enter project name:")
        if input_name == "":
            print("Usage: python create_cmake_library.py <project_name>")
            sys.exit(1)
    else:
        input_name = sys.argv[1]

    if os.path.exists(input_name):
        print("Sub project already exists")
        sys.exit(1)

    out_path = os.getcwd() + f"\\{input_name}"
    print(f"Creating project at: {out_path}")
    create_cmake_library_template(input_name)
