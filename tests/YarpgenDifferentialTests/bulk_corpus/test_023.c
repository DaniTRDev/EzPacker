/* Bulk Test 023 (Seed 1195) */
extern signed char var_10;
extern unsigned short var_11;

/*
yarpgen version 2.0 (build 1adb290f on 2026:10:17)
Seed: 1195
Invocation: E:\Repos\EzPacker\cmake-build-debug\bin\yarpgen.exe --std=c -s 1195 --out-dir=E:\Repos\EzPacker\.temp_bulk_gen\seed_1195
*/
#define max(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a > _b ? _a : _b; })
#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a < _b ? _a : _b; })
void test(signed char var_1, _Bool var_3, _Bool var_5, short var_7, int zero) {
    var_10 |= ((/* implicit */signed char) min((var_5), (var_5)));
    var_11 -= ((/* implicit */unsigned short) min((((((((/* implicit */_Bool) var_7)) ? (((/* implicit */int) (signed char)107)) : (((/* implicit */int) (unsigned short)192)))) / (((((/* implicit */_Bool) (signed char)-1)) ? (((/* implicit */int) var_1)) : (1472502528))))), (min((((/* implicit */int) min((((/* implicit */short) var_3)), (var_7)))), (-1472502521)))));
}
