set(EZLIB_BUILD_STATIC ${EZPACKER_BUILD_STATIC})
set(ZYDIS_BUILD_STATIC ${EZPACKER_BUILD_STATIC})

include(FetchContent)

function(AddEzLib)
    set(EZLIB_BUILD_TESTS OFF CACHE BOOL "Builds tests for EzLib" FORCE)
    FetchContent_Declare(EzLib GIT_REPOSITORY https://github.com/DaniTRDev/EzLib)
    FetchContent_MakeAvailable(EzLib)
endfunction()

function(AddZydis)
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
    set(INSTALL_GTEST OFF CACHE BOOL "Installs gtest" FORCE)

    FetchContent_Declare(
            GTest
            GIT_REPOSITORY https://github.com/google/googletest
            GIT_TAG 52eb8108c5bdec04579160ae17225d66034bd723
    )
    FetchContent_MakeAvailable(GTest)
endfunction()

AddEzLib()
AddZydis()
AddGTest()