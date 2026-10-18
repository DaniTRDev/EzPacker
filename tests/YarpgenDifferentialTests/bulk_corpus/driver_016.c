/* Bulk Driver 016 (Seed 809) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

short var_5 = (short)29519;
int zero = 0;
signed char var_10 = (signed char)-89;
signed char var_11 = (signed char)124;
void init() {
}

void checksum() {
    hash(&seed, var_10);
    hash(&seed, var_11);
}
void test(short var_5, int zero);

int main() {
    init();
    test(var_5, zero);
    checksum();
    printf("%llu\n", seed);
}
