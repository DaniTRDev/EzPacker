/* Bulk Test 004 (Seed 188) */
extern signed char var_16;
extern signed char var_17;

/*
yarpgen version 2.0 (build 1adb290f on 2026:10:17)
Seed: 188
Invocation: E:\Repos\EzPacker\cmake-build-debug\bin\yarpgen.exe --std=c -s 188 --out-dir=E:\Repos\EzPacker\.temp_bulk_gen\seed_188
*/
#define max(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a > _b ? _a : _b; })
#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a < _b ? _a : _b; })
void test(unsigned char var_8, long long int var_9, int var_15, int zero) {
    var_16 &= ((/* implicit */signed char) ((unsigned long long int) ((((/* implicit */int) ((((/* implicit */_Bool) (unsigned short)7)) || (((/* implicit */_Bool) var_15))))) << (((max((((/* implicit */long long int) (unsigned short)7)), (6465705057772094257LL))) - (6465705057772094229LL))))));
    var_17 -= ((/* implicit */signed char) ((((/* implicit */_Bool) var_9)) ? (var_15) : (((/* implicit */int) var_8))));
}
