#ifndef _EZPACKER_ERRNO_H
#define _EZPACKER_ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif

extern int errno;

#define EDOM    33
#define ERANGE  34
#define EILSEQ  84
#define EINVAL  22
#define ENOMEM  12
#define ENOENT  2
#define EACCES  13
#define EEXIST  17
#define EIO     5
#define ENOSPC  28

#ifdef __cplusplus
}
#endif

#endif // _EZPACKER_ERRNO_H
