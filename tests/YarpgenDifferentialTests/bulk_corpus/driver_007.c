/* Bulk Driver 007 (Seed 325) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

unsigned short var_3 = (unsigned short)50416;
int zero = 0;
unsigned short var_14 = (unsigned short)19411;
unsigned short var_15 = (unsigned short)6361;
unsigned short var_16 = (unsigned short)37559;
void init() {
}

void checksum() {
    hash(&seed, var_14);
    hash(&seed, var_15);
    hash(&seed, var_16);
}
void test(unsigned short var_3, int zero);

int main() {
    init();
    test(var_3, zero);
    checksum();
    printf("%llu\n", seed);
}
