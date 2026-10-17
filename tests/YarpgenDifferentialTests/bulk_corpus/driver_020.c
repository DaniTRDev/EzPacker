/* Bulk Driver 020 (Seed 1000) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

unsigned int var_4 = 2832433758U;
short var_12 = (short)-17049;
int zero = 0;
unsigned short var_14 = (unsigned short)59089;
unsigned int var_15 = 214044524U;
void init() {
}

void checksum() {
    hash(&seed, var_14);
    hash(&seed, var_15);
}
void test(unsigned int var_4, short var_12, int zero);

int main() {
    init();
    test(var_4, var_12, zero);
    checksum();
    printf("%llu\n", seed);
}
