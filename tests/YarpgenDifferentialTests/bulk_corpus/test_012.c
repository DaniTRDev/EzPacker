/* Bulk Test 012 (Seed 620) */
extern signed char var_19;
extern signed char var_20;

/*
yarpgen version 2.0 (build 1adb290f on 2026:10:17)
Seed: 620
Invocation: E:\Repos\EzPacker\cmake-build-debug\bin\yarpgen.exe --std=c -s 620 --out-dir=E:\Repos\EzPacker\.temp_bulk_gen\seed_620
*/
#define max(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a > _b ? _a : _b; })
#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a < _b ? _a : _b; })
void test(unsigned short var_1, signed char var_2, signed char var_3, unsigned short var_5, unsigned short var_6, signed char var_11, signed char var_18, int zero) {
    var_19 &= ((/* implicit */signed char) ((min((((((/* implicit */int) (signed char)10)) - (((/* implicit */int) (unsigned short)65535)))), (((((/* implicit */_Bool) (unsigned short)64734)) ? (((/* implicit */int) var_18)) : (((/* implicit */int) var_5)))))) <= (((((/* implicit */_Bool) ((((/* implicit */_Bool) var_6)) ? (((/* implicit */int) (signed char)-1)) : (((/* implicit */int) var_3))))) ? (((/* implicit */int) min((var_11), (var_2)))) : (((((/* implicit */_Bool) (signed char)-123)) ? (((/* implicit */int) var_2)) : (((/* implicit */int) var_1))))))));
    var_20 = var_11;
}
