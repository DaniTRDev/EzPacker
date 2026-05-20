EzCMK_GetLibraryBuildType(BUILD_TYPE)

if (BUILD_TYPE EQUAL "STATIC")
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Builds sub projects as static libraries" FORCE)
    set(EZLIB_BUILD_STATIC ON)
    set(ZYDIS_BUILD_STATIC ON)
else ()
    set(BUILD_SHARED_LIBS ON CACHE BOOL "Builds sub projects as shared libraries" FORCE)
    set(EZLIB_BUILD_STATIC OFF)
    set(ZYDIS_BUILD_STATIC OFF)
endif ()

set(INSTALL_GTEST OFF CACHE BOOL "Installs gtest" FORCE)

EzCMK_AddVendor(https://github.com/zyantific/zasm
        813a1875d1f45d6c27d938eb10b5f96c05757a1b
        zasm
)

set(ASMJIT_STATIC ON CACHE BOOL "" FORCE)
EzCMK_AddVendor(https://github.com/asmjit/asmjit.git
        0bd5787b54b575ed94bf32ac452153b34385c514
        asmjit
)

EzCMK_AddVendor(https://github.com/google/googletest
        52eb8108c5bdec04579160ae17225d66034bd723
        GTest
)

EzCMK_AddVendor(https://github.com/libtom/libtommath
        994f6df64cc9dd86a4f7994fe0530861470b5fa4
        libtommath
)

EzCMK_AddVendor(https://github.com/glfw/glfw
        master
        glfw
)

# ImGui doesn't have CMakeLists by default, but we can fetch it and build its sources manually.
FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

find_package(OpenGL REQUIRED)