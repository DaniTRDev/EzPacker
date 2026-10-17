/* Bulk Driver 024 (Seed 1261) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

_Bool var_1 = (_Bool)1;
unsigned char var_8 = (unsigned char)175;
int zero = 0;
signed char var_15 = (signed char)-81;
unsigned long long int var_16 = 6209634927714336846ULL;
short var_17 = (short)-580;
void init() {
}

void checksum() {
    hash(&seed, var_15);
    hash(&seed, var_16);
    hash(&seed, var_17);
}
void test(_Bool var_1, unsigned char var_8, int zero);

int main() {
    init();
    test(var_1, var_8, zero);
    checksum();
    printf("%llu\n", seed);
}
