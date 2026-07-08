# File used to build LibTomFloat with CMake
message(STATUS "Building LibTomFloat with CMake")

set(LIBTOMFLOAT_ROOT_DIR "${libtomfloat_SOURCE_DIR}")

# Define relative paths to keep things readable
set(RELATIVE_FILES
        "tomfloat.h"
        "mpf_init.c" "mpf_clear.c" "mpf_init_multi.c" "mpf_clear_multi.c" "mpf_init_copy.c"
        "mpf_copy.c" "mpf_exch.c" "mpf_abs.c" "mpf_neg.c"
        "mpf_cmp.c" "mpf_cmp_d.c"
        "mpf_normalize.c" "mpf_normalize_to.c" "mpf_iterations.c"
        "mpf_const_0.c"    "mpf_const_1r2.c"  "mpf_const_2rpi.c"  "mpf_const_e.c"
        "mpf_const_l2e.c"  "mpf_const_pi.c"   "mpf_const_pi4.c"   "mpf_const_1pi.c"
        "mpf_const_2pi.c"  "mpf_const_d.c"    "mpf_const_l10e.c"  "mpf_const_le2.c"
        "mpf_const_pi2.c"  "mpf_const_r2.c"   "mpf_const_ln_d.c"  "mpf_const_sqrt_d.c" # <-- Added here
        "mpf_mul_2.c" "mpf_div_2.c" "mpf_add.c" "mpf_sub.c" "mpf_mul.c" "mpf_sqr.c" "mpf_div.c"
        "mpf_add_d.c" "mpf_sub_d.c" "mpf_mul_d.c" "mpf_div_d.c"
        "mpf_invsqrt.c" "mpf_inv.c" "mpf_exp.c" "mpf_sqrt.c" "mpf_pow.c" "mpf_ln.c"
        "mpf_cos.c" "mpf_sin.c" "mpf_tan.c" "mpf_acos.c" "mpf_asin.c" "mpf_atan.c"
)

set(LIBTOMFLOAT_FILES_LIST "")
foreach(FILE ${RELATIVE_FILES})
    list(APPEND LIBTOMFLOAT_FILES_LIST "${LIBTOMFLOAT_ROOT_DIR}/${FILE}")
endforeach()

EzCMK_ConfigureLib(NAME "tomfloat"
        FILE_LIST ${LIBTOMFLOAT_FILES_LIST}
        LINKED_LIBRARIES "libtommath"
        INCLUDED_DIRS "${LIBTOMFLOAT_ROOT_DIR}"
)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(tomfloat PRIVATE -Os -Wall -W)
endif()

add_library(libtomfloat::tomfloat ALIAS tomfloat)