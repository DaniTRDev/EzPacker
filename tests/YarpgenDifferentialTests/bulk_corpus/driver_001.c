/* Bulk Driver 001 (Seed 121) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

unsigned char var_1 = (unsigned char)196;
unsigned short var_2 = (unsigned short)40847;
unsigned char var_7 = (unsigned char)19;
int zero = 0;
unsigned int var_12 = 4126015626U;
signed char var_13 = (signed char)-104;
void init() {
}

void checksum() {
    hash(&seed, var_12);
    hash(&seed, var_13);
}
void test(unsigned char var_1, unsigned short var_2, unsigned char var_7, int zero);

int main() {
    init();
    test(var_1, var_2, var_7, zero);
    checksum();
    printf("%llu\n", seed);
}
