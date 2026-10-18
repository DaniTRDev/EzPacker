/* Bulk Driver 018 (Seed 936) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

signed char var_3 = (signed char)70;
signed char var_10 = (signed char)37;
unsigned int var_12 = 2491977485U;
int zero = 0;
signed char var_13 = (signed char)88;
int var_14 = 1746897320;
int var_15 = 1004756332;
void init() {
}

void checksum() {
    hash(&seed, var_13);
    hash(&seed, var_14);
    hash(&seed, var_15);
}
void test(signed char var_3, signed char var_10, unsigned int var_12, int zero);

int main() {
    init();
    test(var_3, var_10, var_12, zero);
    checksum();
    printf("%llu\n", seed);
}
