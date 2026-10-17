/* Bulk Test 019 (Seed 948) */
extern unsigned int var_20;
extern long long int var_21;
extern unsigned char var_22;

/*
yarpgen version 2.0 (build 1adb290f on 2026:10:17)
Seed: 948
Invocation: E:\Repos\EzPacker\cmake-build-debug\bin\yarpgen.exe --std=c -s 948 --out-dir=E:\Repos\EzPacker\.temp_bulk_gen\seed_948
*/
#define max(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a > _b ? _a : _b; })
#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a < _b ? _a : _b; })
void test(unsigned char var_1, signed char var_2, signed char var_4, short var_8, unsigned char var_10, long long int var_12, unsigned char var_13, unsigned long long int var_14, short var_16, _Bool var_18, int zero) {
    var_20 -= ((/* implicit */unsigned int) ((((/* implicit */int) var_2)) <= (((((/* implicit */int) var_13)) ^ (((/* implicit */int) var_1))))));
    var_21 ^= ((/* implicit */long long int) ((((/* implicit */int) ((((/* implicit */int) max((var_8), (((/* implicit */short) var_10))))) == ((~(((/* implicit */int) var_13))))))) >> (((((unsigned int) ((((/* implicit */unsigned long long int) ((/* implicit */int) var_13))) & (var_14)))) - (19U)))));
    var_22 |= ((/* implicit */unsigned char) ((min((((((/* implicit */unsigned long long int) ((/* implicit */int) var_18))) - (var_14))), (((/* implicit */unsigned long long int) ((((/* implicit */int) var_1)) << (((((/* implicit */int) var_4)) - (92)))))))) - (((/* implicit */unsigned long long int) ((/* implicit */int) (((~(var_12))) >= (((/* implicit */long long int) ((/* implicit */int) var_16)))))))));
}
