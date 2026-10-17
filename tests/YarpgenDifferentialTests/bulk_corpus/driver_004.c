/* Bulk Driver 004 (Seed 188) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

unsigned char var_8 = (unsigned char)184;
long long int var_9 = -570384236160678441LL;
int var_15 = 315635999;
int zero = 0;
signed char var_16 = (signed char)71;
signed char var_17 = (signed char)-120;
void init() {
}

void checksum() {
    hash(&seed, var_16);
    hash(&seed, var_17);
}
void test(unsigned char var_8, long long int var_9, int var_15, int zero);

int main() {
    init();
    test(var_8, var_9, var_15, zero);
    checksum();
    printf("%llu\n", seed);
}
