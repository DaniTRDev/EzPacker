/* Bulk Driver 011 (Seed 592) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

int var_0 = -2080641052;
_Bool var_5 = (_Bool)1;
_Bool var_8 = (_Bool)0;
_Bool var_11 = (_Bool)0;
int var_12 = 785229881;
unsigned char var_13 = (unsigned char)86;
int zero = 0;
_Bool var_15 = (_Bool)0;
_Bool var_16 = (_Bool)1;
_Bool var_17 = (_Bool)0;
int var_18 = 834386569;
void init() {
}

void checksum() {
    hash(&seed, var_15);
    hash(&seed, var_16);
    hash(&seed, var_17);
    hash(&seed, var_18);
}
void test(int var_0, _Bool var_5, _Bool var_8, _Bool var_11, int var_12, unsigned char var_13, int zero);

int main() {
    init();
    test(var_0, var_5, var_8, var_11, var_12, var_13, zero);
    checksum();
    printf("%llu\n", seed);
}
