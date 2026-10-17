/* Bulk Driver 009 (Seed 483) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

signed char var_0 = (signed char)-61;
unsigned short var_7 = (unsigned short)17290;
int zero = 0;
unsigned short var_13 = (unsigned short)6844;
unsigned short var_14 = (unsigned short)62221;
void init() {
}

void checksum() {
    hash(&seed, var_13);
    hash(&seed, var_14);
}
void test(signed char var_0, unsigned short var_7, int zero);

int main() {
    init();
    test(var_0, var_7, zero);
    checksum();
    printf("%llu\n", seed);
}
