import os

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
    common_header_content = f"""#pragma once

// {project_name} Common Header
// This will be used as the precompiled header

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
"""
    with open(f"{project_name}/include/{project_name}Common.h", "w") as f:
        f.write(common_header_content)

    main_header_content = f"""#pragma once

// Main {project_name} header file

#include "{project_name}Common.h"

// Your library interface goes here
"""
    with open(f"{project_name}/include/{project_name}.h", "w") as f:
        f.write(main_header_content)

    # Create sample source file
    sample_src_content = f"""#include "{project_name}.h"

// Your implementation goes here
"""
    with open(f"{project_name}/src/{project_name}.cpp", "w") as f:
        f.write(sample_src_content)

    # Create test file
    test_content = f"""#include <gtest/gtest.h>
#include "{project_name}.h"

TEST({project_name}Test, BasicTest) {{
    // Your test code here
    EXPECT_TRUE(true);
}}
"""
    with open(f"{project_name}/tests/Test{project_name}.cpp", "w") as f:
        f.write(test_content)

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
            "src/{project_name}.cpp"
    )

    # Set precompiled header
    target_precompile_headers({project_name} PUBLIC "${{CMAKE_CURRENT_SOURCE_DIR}}/include/{project_name}Common.h")

    # Add include directories and link dependencies
    target_link_libraries({project_name} PUBLIC EzLogger gtest gtest_main)
    target_include_directories({project_name} PUBLIC "${{CMAKE_CURRENT_SOURCE_DIR}}/include" ${{gtest_build_include_dirs}})
endfunction()

Build{project_name}()

# Add tests
if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for {project_name}")

    add_custom_target(CopyTestFiles ALL
            COMMAND ${{CMAKE_COMMAND}} -E copy_directory_if_different
            "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/testData"
            "$<TARGET_FILE_DIR:{project_name}>/testData"
            COMMENT "Copying test data"
    )

    EzPacker_AddTest(
            "T_{project_name}Test"
            ""
            "{project_name}"
            ""
            "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/Test{project_name}.cpp"
    )
endif()
"""
    with open(f"{project_name}/CMakeLists.txt", "w") as f:
        f.write(cmake_content)

    print(f"Successfully created {project_name} project structure and CMake configuration.")


if __name__ == "__main__":
    import sys

    if len(sys.argv) != 2:
        print("Usage: python create_cmake_library.py <project_name>")
        sys.exit(1)

    project_name = input('Project name: ')
    if os.path.exists(project_name):
        print("Sub project already exists")
        sys.exit(1)


    create_cmake_library_template(project_name)
