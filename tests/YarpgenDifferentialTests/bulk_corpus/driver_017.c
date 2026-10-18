/* Bulk Driver 017 (Seed 819) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

unsigned int var_2 = 802186624U;
unsigned char var_8 = (unsigned char)203;
int zero = 0;
_Bool var_13 = (_Bool)0;
unsigned char var_14 = (unsigned char)60;
_Bool var_15 = (_Bool)1;
void init() {
}

void checksum() {
    hash(&seed, var_13);
    hash(&seed, var_14);
    hash(&seed, var_15);
}
void test(unsigned int var_2, unsigned char var_8, int zero);

int main() {
    init();
    test(var_2, var_8, zero);
    checksum();
    printf("%llu\n", seed);
}
