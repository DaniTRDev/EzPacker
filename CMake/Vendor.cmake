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