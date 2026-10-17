/* Bulk Driver 022 (Seed 1104) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

signed char var_1 = (signed char)-86;
unsigned long long int var_3 = 10569364543964151369ULL;
long long int var_5 = 8879804587367635674LL;
short var_8 = (short)-14000;
unsigned short var_9 = (unsigned short)45469;
int zero = 0;
unsigned int var_10 = 1286609927U;
unsigned long long int var_11 = 10969374599806294600ULL;
void init() {
}

void checksum() {
    hash(&seed, var_10);
    hash(&seed, var_11);
}
void test(signed char var_1, unsigned long long int var_3, long long int var_5, short var_8, unsigned short var_9, int zero);

int main() {
    init();
    test(var_1, var_3, var_5, var_8, var_9, zero);
    checksum();
    printf("%llu\n", seed);
}
