/* Bulk Test 015 (Seed 740) */
extern signed char var_16;
extern signed char var_17;

/*
yarpgen version 2.0 (build 1adb290f on 2026:10:17)
Seed: 740
Invocation: E:\Repos\EzPacker\cmake-build-debug\bin\yarpgen.exe --std=c -s 740 --out-dir=E:\Repos\EzPacker\.temp_bulk_gen\seed_740
*/
#define max(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a > _b ? _a : _b; })
#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a < _b ? _a : _b; })
void test(unsigned char var_1, signed char var_4, unsigned int var_5, int var_11, unsigned int var_13, int zero) {
    var_16 *= ((/* implicit */signed char) ((((/* implicit */_Bool) var_13)) ? (((/* implicit */unsigned int) var_11)) : ((~(((((/* implicit */unsigned int) ((/* implicit */int) var_4))) * (var_5)))))));
    var_17 &= ((/* implicit */signed char) var_1);
}
