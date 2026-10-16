#ifndef CHIBICC_SHIM_UNISTD_H
#define CHIBICC_SHIM_UNISTD_H

#ifdef _WIN32
#include <io.h>
#include <process.h>
#include <direct.h>

#ifndef R_OK
#define R_OK 4
#endif
#ifndef W_OK
#define W_OK 2
#endif
#ifndef F_OK
#define F_OK 0
#endif

#else
#include_next <unistd.h>
#endif

#endif // CHIBICC_SHIM_UNISTD_H
