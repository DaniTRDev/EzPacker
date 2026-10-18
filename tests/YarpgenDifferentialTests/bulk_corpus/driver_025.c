/* Bulk Driver 025 (Seed 1363) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

unsigned long long int var_0 = 8509669532733739739ULL;
unsigned long long int var_1 = 9643961003897826578ULL;
unsigned int var_2 = 618902832U;
unsigned char var_5 = (unsigned char)245;
unsigned long long int var_6 = 484930110179475611ULL;
long long int var_9 = -1213325901896946471LL;
int zero = 0;
int var_12 = -1645356415;
_Bool var_13 = (_Bool)1;
void init() {
}

void checksum() {
    hash(&seed, var_12);
    hash(&seed, var_13);
}
void test(unsigned long long int var_0, unsigned long long int var_1, unsigned int var_2, unsigned char var_5, unsigned long long int var_6, long long int var_9, int zero);

int main() {
    init();
    test(var_0, var_1, var_2, var_5, var_6, var_9, zero);
    checksum();
    printf("%llu\n", seed);
}
