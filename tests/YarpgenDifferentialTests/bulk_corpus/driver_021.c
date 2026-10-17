/* Bulk Driver 021 (Seed 1086) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

unsigned long long int var_4 = 2932427897000517413ULL;
unsigned int var_5 = 2212204685U;
unsigned char var_6 = (unsigned char)58;
unsigned char var_15 = (unsigned char)45;
int zero = 0;
unsigned long long int var_18 = 14001425243492419481ULL;
int var_19 = 1724582537;
void init() {
}

void checksum() {
    hash(&seed, var_18);
    hash(&seed, var_19);
}
void test(unsigned long long int var_4, unsigned int var_5, unsigned char var_6, unsigned char var_15, int zero);

int main() {
    init();
    test(var_4, var_5, var_6, var_15, zero);
    checksum();
    printf("%llu\n", seed);
}
