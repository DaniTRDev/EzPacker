#ifndef _STDARG_H
#define _STDARG_H

#ifdef __chibicc__

#define __NEED_va_list
#define __NEED___isoc_va_list
#include <bits/alltypes.h>

#define va_start(ap, last) \
    do { *(ap) = *(__va_elem *)__va_area__; } while (0)

#define va_end(ap) do { } while (0)

#define va_copy(dest, src) ((dest)[0] = (src)[0])

static inline void *__va_arg_mem(__va_elem *ap, int sz, int align) {
    void *p = ap->overflow_arg_area;
    if (align > 8)
        p = (void *)(((unsigned long long)p + 15) / 16 * 16);
    ap->overflow_arg_area = (void *)(((unsigned long long)p + sz + 7) / 8 * 8);
    return p;
}

static inline void *__va_arg_gp(__va_elem *ap, int sz, int align) {
    if (ap->gp_offset >= 48)
        return __va_arg_mem(ap, sz, align);
    void *r = (char *)ap->reg_save_area + ap->gp_offset;
    ap->gp_offset += 8;
    return r;
}

static inline void *__va_arg_fp(__va_elem *ap, int sz, int align) {
    if (ap->fp_offset >= 112)
        return __va_arg_mem(ap, sz, align);
    void *r = (char *)ap->reg_save_area + ap->fp_offset;
    ap->fp_offset += 8;
    return r;
}

#define va_arg(ap, ty) \
    ({ \
        int __klass = __builtin_reg_class(ty); \
        *(ty *)(__klass == 0 ? __va_arg_gp(ap, sizeof(ty), _Alignof(ty)) : \
                __klass == 1 ? __va_arg_fp(ap, sizeof(ty), _Alignof(ty)) : \
                __va_arg_mem(ap, sizeof(ty), _Alignof(ty))); \
    })

#define __GNUC_VA_LIST 1
typedef va_list __gnuc_va_list;

#else

#ifdef __cplusplus
extern "C" {
#endif

#define __NEED_va_list
#include <bits/alltypes.h>

#define va_start(v, l)  __builtin_va_start(v, l)
#define va_end(v)       __builtin_va_end(v)
#define va_arg(v, l)    __builtin_va_arg(v, l)
#define va_copy(d, s)   __builtin_va_copy(d, s)

#ifdef __cplusplus
}
#endif

#endif /* __chibicc__ */

#endif /* _STDARG_H */
