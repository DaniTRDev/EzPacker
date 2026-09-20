#ifndef EZCORE_LIBBF_WRAPPER_H
#define EZCORE_LIBBF_WRAPPER_H

/**
 * Single point through which the C LibBF header is included.
 *
 * LibBF exports very generic C symbols (bf_*, limb_t, ...) that would otherwise collide with
 * standard math and platform symbols, so the whole header is wrapped in the libbf namespace.
 * Because libbf.h has its own include guard, every translation unit must go through this wrapper
 * first: including <libbf.h> directly before this header would leave the names at global scope
 * and break the `libbf::` references used by FlexFloat. Keep all LibBF includes routed here.
 */
extern "C"
{
    namespace libbf
    {
#include <libbf.h>
    };
}

#endif // EZCORE_LIBBF_WRAPPER_H
