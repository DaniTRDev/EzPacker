/* Bulk Driver 015 (Seed 740) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

unsigned char var_1 = (unsigned char)196;
signed char var_4 = (signed char)23;
unsigned int var_5 = 1264920553U;
int var_11 = 1098903559;
unsigned int var_13 = 3993403053U;
int zero = 0;
signed char var_16 = (signed char)121;
signed char var_17 = (signed char)12;
void init() {
}

void checksum() {
    hash(&seed, var_16);
    hash(&seed, var_17);
}
void test(unsigned char var_1, signed char var_4, unsigned int var_5, int var_11, unsigned int var_13, int zero);

int main() {
    init();
    test(var_1, var_4, var_5, var_11, var_13, zero);
    checksum();
    printf("%llu\n", seed);
}
