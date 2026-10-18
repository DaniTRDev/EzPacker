/* Bulk Driver 002 (Seed 132) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

unsigned short var_5 = (unsigned short)7046;
int zero = 0;
short var_16 = (short)8951;
unsigned short var_17 = (unsigned short)971;
void init() {
}

void checksum() {
    hash(&seed, var_16);
    hash(&seed, var_17);
}
void test(unsigned short var_5, int zero);

int main() {
    init();
    test(var_5, zero);
    checksum();
    printf("%llu\n", seed);
}
