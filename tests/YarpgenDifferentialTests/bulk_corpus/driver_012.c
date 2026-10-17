/* Bulk Driver 012 (Seed 620) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

unsigned short var_1 = (unsigned short)51879;
signed char var_2 = (signed char)19;
signed char var_3 = (signed char)77;
unsigned short var_5 = (unsigned short)12727;
unsigned short var_6 = (unsigned short)7520;
signed char var_11 = (signed char)-49;
signed char var_18 = (signed char)-72;
int zero = 0;
signed char var_19 = (signed char)115;
signed char var_20 = (signed char)-9;
void init() {
}

void checksum() {
    hash(&seed, var_19);
    hash(&seed, var_20);
}
void test(unsigned short var_1, signed char var_2, signed char var_3, unsigned short var_5, unsigned short var_6, signed char var_11, signed char var_18, int zero);

int main() {
    init();
    test(var_1, var_2, var_3, var_5, var_6, var_11, var_18, zero);
    checksum();
    printf("%llu\n", seed);
}
