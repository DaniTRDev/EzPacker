/* Bulk Driver 014 (Seed 712) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

unsigned int var_5 = 32126858U;
unsigned int var_7 = 2146030206U;
unsigned int var_9 = 1105146202U;
int zero = 0;
unsigned int var_18 = 1975638205U;
unsigned int var_19 = 2388697381U;
void init() {
}

void checksum() {
    hash(&seed, var_18);
    hash(&seed, var_19);
}
void test(unsigned int var_5, unsigned int var_7, unsigned int var_9, int zero);

int main() {
    init();
    test(var_5, var_7, var_9, zero);
    checksum();
    printf("%llu\n", seed);
}
