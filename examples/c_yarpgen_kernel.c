// EzPacker C Frontend: YARPGen Arithmetic & Control Flow Kernel
// Demonstrates direct compilation of C11 source code through the embedded chibicc frontend into SSA MIR,
// featuring scalar arithmetic, bitwise logic, ternary operators, and conditional branching.

extern unsigned int checksum_val;

unsigned int hash_step(unsigned int acc, unsigned int val) {
    acc ^= val + 0x9e3779b9 + (acc << 6) + (acc >> 2);
    return acc;
}

int kernel_compute(int a, int b, int c) {
    int x = (a > b) ? (a - b) : (b - a);
    int y = (b > c) ? (b ^ c) : (b + c);
    int z = (x > y) ? (x * 3) : (y / 2);
    
    unsigned int h = 0;
    h = hash_step(h, (unsigned int)x);
    h = hash_step(h, (unsigned int)y);
    h = hash_step(h, (unsigned int)z);
    
    checksum_val = h;
    return z;
}
