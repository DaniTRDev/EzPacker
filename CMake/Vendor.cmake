EzCMK_GetLibraryBuildType(BUILD_TYPE)

if (BUILD_TYPE EQUAL "STATIC")
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Builds sub projects as static libraries" FORCE)
    set(EZLIB_BUILD_STATIC ON)
else ()
    set(BUILD_SHARED_LIBS ON CACHE BOOL "Builds sub projects as shared libraries" FORCE)
    set(EZLIB_BUILD_STATIC OFF)
endif ()

set(INSTALL_GTEST OFF CACHE BOOL "Installs gtest" FORCE)

EzCMK_AddVendor(https://github.com/google/googletest
        52eb8108c5bdec04579160ae17225d66034bd723
        GTest
)

EzCMK_AddVendor(https://github.com/libtom/libtommath
        994f6df64cc9dd86a4f7994fe0530861470b5fa4
        libtommath
)

EzCMK_AddVendor(https://github.com/foonathan/lexy
        c1358c4117752393e9ce27d6c885d9146846f2f9
        lexy
)

EzCMK_AddVendor(https://github.com/p-ranav/argparse
        d924b84eba1f0f0adf38b20b7b4829f6f65b6570
        argparse
)

FetchContent_Declare(
        chibicc
        GIT_REPOSITORY https://github.com/rui314/chibicc
        GIT_TAG 90d1f7f199cc55b13c7fdb5839d1409806633fdb
        PATCH_COMMAND ${CMAKE_COMMAND} -E copy_directory
                      "${CMAKE_CURRENT_SOURCE_DIR}/CMake/chibicc/overlay"
                      "<SOURCE_DIR>"
)

EzCMK_AddVendor(https://github.com/rui314/chibicc
        90d1f7f199cc55b13c7fdb5839d1409806633fdb
        chibicc
)