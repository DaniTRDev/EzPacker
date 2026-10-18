/* Bulk Driver 019 (Seed 948) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

unsigned char var_1 = (unsigned char)255;
signed char var_2 = (signed char)124;
signed char var_4 = (signed char)110;
short var_8 = (short)24768;
unsigned char var_10 = (unsigned char)102;
long long int var_12 = 5317097084226490321LL;
unsigned char var_13 = (unsigned char)115;
unsigned long long int var_14 = 6256942894840308536ULL;
short var_16 = (short)-29289;
_Bool var_18 = (_Bool)1;
int zero = 0;
unsigned int var_20 = 537580122U;
long long int var_21 = 1503369135116882043LL;
unsigned char var_22 = (unsigned char)137;
void init() {
}

void checksum() {
    hash(&seed, var_20);
    hash(&seed, var_21);
    hash(&seed, var_22);
}
void test(unsigned char var_1, signed char var_2, signed char var_4, short var_8, unsigned char var_10, long long int var_12, unsigned char var_13, unsigned long long int var_14, short var_16, _Bool var_18, int zero);

int main() {
    init();
    test(var_1, var_2, var_4, var_8, var_10, var_12, var_13, var_14, var_16, var_18, zero);
    checksum();
    printf("%llu\n", seed);
}
