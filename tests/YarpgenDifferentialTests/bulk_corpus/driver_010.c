/* Bulk Driver 010 (Seed 484) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

signed char var_5 = (signed char)97;
signed char var_12 = (signed char)-57;
int zero = 0;
unsigned char var_18 = (unsigned char)158;
_Bool var_19 = (_Bool)0;
void init() {
}

void checksum() {
    hash(&seed, var_18);
    hash(&seed, var_19);
}
void test(signed char var_5, signed char var_12, int zero);

int main() {
    init();
    test(var_5, var_12, zero);
    checksum();
    printf("%llu\n", seed);
}
