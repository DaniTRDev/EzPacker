/* Bulk Driver 013 (Seed 650) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

unsigned long long int var_0 = 8757786102262041253ULL;
unsigned int var_5 = 1564393566U;
int zero = 0;
unsigned long long int var_10 = 1745315581803348738ULL;
int var_11 = 1364134365;
void init() {
}

void checksum() {
    hash(&seed, var_10);
    hash(&seed, var_11);
}
void test(unsigned long long int var_0, unsigned int var_5, int zero);

int main() {
    init();
    test(var_0, var_5, zero);
    checksum();
    printf("%llu\n", seed);
}
