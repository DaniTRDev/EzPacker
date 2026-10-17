/* Bulk Driver 006 (Seed 280) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

signed char var_1 = (signed char)-118;
unsigned int var_2 = 324544302U;
unsigned short var_5 = (unsigned short)21476;
unsigned long long int var_6 = 8532197491202054089ULL;
unsigned int var_7 = 462092270U;
unsigned int var_10 = 77960737U;
long long int var_12 = -8490248227359797227LL;
int zero = 0;
unsigned long long int var_15 = 8400858962963402497ULL;
unsigned short var_16 = (unsigned short)63940;
void init() {
}

void checksum() {
    hash(&seed, var_15);
    hash(&seed, var_16);
}
void test(signed char var_1, unsigned int var_2, unsigned short var_5, unsigned long long int var_6, unsigned int var_7, unsigned int var_10, long long int var_12, int zero);

int main() {
    init();
    test(var_1, var_2, var_5, var_6, var_7, var_10, var_12, zero);
    checksum();
    printf("%llu\n", seed);
}
