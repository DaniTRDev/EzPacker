if (EZPACKER_BUILD_STATIC)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Builds sub projects as static libraries" FORCE)
    set(EZLIB_BUILD_STATIC ON)
    set(ZYDIS_BUILD_STATIC ON)
else ()
    set(BUILD_SHARED_LIBS ON CACHE BOOL "Builds sub projects as shared libraries" FORCE)
    set(EZLIB_BUILD_STATIC OFF)
    set(ZYDIS_BUILD_STATIC OFF)
endif ()

include(FetchContent)

function(AddEzLib)
    message(STATUS "Retrieving EzLib")

    set(EZLIB_BUILD_TESTS OFF CACHE BOOL "Builds tests for EzLib" FORCE)
    FetchContent_Declare(EzLib
            GIT_REPOSITORY https://github.com/DaniTRDev/EzLib
            GIT_TAG fcf99d560d9a94b7456191a001d3c7f345be6176
    )
    FetchContent_MakeAvailable(EzLib)
endfunction()

function(AddZydis)
    message(STATUS "Retrieving Zydis")

    if (ZYDIS_BUILD_STATIC)
        set(ZYDIS_BUILD_SHARED_LIB OFF CACHE BOOL "Builds Zydis shared" FORCE)
    endif ()

    FetchContent_Declare(
            Zydis
            GIT_REPOSITORY https://github.com/zyantific/zydis
            GIT_TAG a2278f1d254e492f6a6b39f6cb5d1f5d515659dc
    )
    FetchContent_MakeAvailable(Zydis)
endfunction()

function(AddGTest)
    message(STATUS "Retrieving GTest")

    set(INSTALL_GTEST OFF CACHE BOOL "Installs gtest" FORCE)
    FetchContent_Declare(
            GTest
            GIT_REPOSITORY https://github.com/google/googletest
            GIT_TAG 52eb8108c5bdec04579160ae17225d66034bd723
    )
    FetchContent_MakeAvailable(GTest)
endfunction()

function(AddJsonLib)
    message(STATUS "Retrieving nlohman::json")
    FetchContent_Declare(
            json
            GIT_REPOSITORY https://github.com/nlohmann/json
            GIT_TAG 3cca3ad21012e289d34970e2e3060d255494c548
    )
    FetchContent_MakeAvailable(json)
endfunction()

function(addLibTomMath)
    message(STATUS "Retrieving libtommath")
    FetchContent_Declare(
            libtommath
            GIT_REPOSITORY https://github.com/libtom/libtommath
            GIT_TAG 994f6df64cc9dd86a4f7994fe0530861470b5fa4
    )
    FetchContent_MakeAvailable(libtommath)
endfunction()

AddEzLib()
AddZydis()
AddGTest()
AddJsonLib()
addLibTomMath()