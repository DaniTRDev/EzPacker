/* Bulk Driver 023 (Seed 1195) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

signed char var_1 = (signed char)-26;
_Bool var_3 = (_Bool)1;
_Bool var_5 = (_Bool)1;
short var_7 = (short)27046;
int zero = 0;
signed char var_10 = (signed char)-34;
unsigned short var_11 = (unsigned short)34747;
void init() {
}

void checksum() {
    hash(&seed, var_10);
    hash(&seed, var_11);
}
void test(signed char var_1, _Bool var_3, _Bool var_5, short var_7, int zero);

int main() {
    init();
    test(var_1, var_3, var_5, var_7, zero);
    checksum();
    printf("%llu\n", seed);
}
