/* Bulk Driver 005 (Seed 231) */
#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
    *seed ^= v + 0x9e3779b9 + ((*seed)<<6) + ((*seed)>>2);
}

unsigned long long int var_2 = 3025466281895703858ULL;
short var_10 = (short)-25155;
int zero = 0;
unsigned short var_16 = (unsigned short)27827;
unsigned long long int var_17 = 7136537552955133349ULL;
void init() {
}

void checksum() {
    hash(&seed, var_16);
    hash(&seed, var_17);
}
void test(unsigned long long int var_2, short var_10, int zero);

int main() {
    init();
    test(var_2, var_10, zero);
    checksum();
    printf("%llu\n", seed);
}
