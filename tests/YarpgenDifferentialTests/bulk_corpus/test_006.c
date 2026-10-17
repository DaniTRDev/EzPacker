/* Bulk Test 006 (Seed 280) */
extern unsigned long long int var_15;
extern unsigned short var_16;

/*
yarpgen version 2.0 (build 1adb290f on 2026:10:17)
Seed: 280
Invocation: E:\Repos\EzPacker\cmake-build-debug\bin\yarpgen.exe --std=c -s 280 --out-dir=E:\Repos\EzPacker\.temp_bulk_gen\seed_280
*/
#define max(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a > _b ? _a : _b; })
#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a < _b ? _a : _b; })
void test(signed char var_1, unsigned int var_2, unsigned short var_5, unsigned long long int var_6, unsigned int var_7, unsigned int var_10, long long int var_12, int zero) {
    var_15 -= ((/* implicit */unsigned long long int) ((unsigned short) ((((/* implicit */_Bool) var_10)) ? (((/* implicit */unsigned long long int) ((/* implicit */int) var_5))) : (((((/* implicit */_Bool) var_2)) ? (var_6) : (((/* implicit */unsigned long long int) var_12)))))));
    var_16 += ((/* implicit */unsigned short) min((((/* implicit */unsigned int) (((!(((/* implicit */_Bool) var_1)))) || (((/* implicit */_Bool) var_10))))), (var_7)));
}
