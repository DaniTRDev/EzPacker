/* Bulk Test 020 (Seed 1000) */
extern unsigned short var_14;
extern unsigned int var_15;

/*
yarpgen version 2.0 (build 1adb290f on 2026:10:17)
Seed: 1000
Invocation: E:\Repos\EzPacker\cmake-build-debug\bin\yarpgen.exe --std=c -s 1000 --out-dir=E:\Repos\EzPacker\.temp_bulk_gen\seed_1000
*/
#define max(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a > _b ? _a : _b; })
#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a < _b ? _a : _b; })
void test(unsigned int var_4, short var_12, int zero) {
    var_14 *= ((/* implicit */unsigned short) -1205319796);
    var_15 ^= ((/* implicit */unsigned int) (-(min((((/* implicit */int) var_12)), (((((/* implicit */_Bool) var_4)) ? (-1205319802) : (((/* implicit */int) (signed char)24))))))));
}
