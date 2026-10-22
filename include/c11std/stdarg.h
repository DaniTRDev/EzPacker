#ifndef _EZPACKER_STDARG_H
#define _EZPACKER_STDARG_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) && !defined(__chibicc__)
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_end(ap)         __builtin_va_end(ap)
#define va_copy(dest, src) __builtin_va_copy(dest, src)
#define va_arg(ap, ty)     __builtin_va_arg(ap, ty)
#else
typedef struct {
    unsigned int gp_offset;
    unsigned int fp_offset;
    void *overflow_arg_area;
    void *reg_save_area;
} __va_elem;

typedef __va_elem va_list[1];

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
#endif

#ifdef __cplusplus
}
#endif

#endif // _EZPACKER_STDARG_H
