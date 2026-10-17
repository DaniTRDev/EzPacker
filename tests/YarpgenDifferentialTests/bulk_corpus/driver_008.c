/* Bulk Driver 008 (Seed 411) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

unsigned short var_2 = (unsigned short)50552;
unsigned int var_3 = 366480066U;
_Bool var_4 = (_Bool)0;
signed char var_5 = (signed char)97;
unsigned long long int var_6 = 16906231707334151326ULL;
unsigned int var_12 = 3847119653U;
int zero = 0;
_Bool var_14 = (_Bool)0;
unsigned char var_15 = (unsigned char)11;
void init() {
}

void checksum() {
    hash(&seed, var_14);
    hash(&seed, var_15);
}
void test(unsigned short var_2, unsigned int var_3, _Bool var_4, signed char var_5, unsigned long long int var_6, unsigned int var_12, int zero);

int main() {
    init();
    test(var_2, var_3, var_4, var_5, var_6, var_12, zero);
    checksum();
    printf("%llu\n", seed);
}
