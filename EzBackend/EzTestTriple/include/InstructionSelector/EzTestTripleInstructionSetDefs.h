#ifndef R
#define R OperandFlag::Read
#define W OperandFlag::Write
#define RW (OperandFlag::Read | OperandFlag::Write)
#define DEFINE_LOCAL_FLAGS
#endif

// =========================================================================
// Data Movement
// =========================================================================
// MOV reg, reg (Op0: Write, Op1: Read)
TARGET_INSTRUCTION(MOV8rr, "mov8rr", 1, { W, R })
TARGET_INSTRUCTION(MOV16rr, "mov16rr", 2, { W, R })
TARGET_INSTRUCTION(MOV32rr, "mov32rr", 3, { W, R })
TARGET_INSTRUCTION(MOV64rr, "mov64rr", 4, { W, R })

// MOV reg, imm (Op0: Write, Op1: Read immediate)
TARGET_INSTRUCTION(MOV8ri, "mov8ri", 5, { W, R })
TARGET_INSTRUCTION(MOV16ri, "mov16ri", 6, { W, R })
TARGET_INSTRUCTION(MOV32ri, "mov32ri", 7, { W, R })
TARGET_INSTRUCTION(MOV64ri, "mov64ri", 8, { W, R })

// LOAD reg, [mem] (Op0: Write dst, Op1: Read base mem)
TARGET_INSTRUCTION(MOV8rm, "mov8rm", 9, { W, R })
TARGET_INSTRUCTION(MOV16rm, "mov16rm", 10, { W, R })
TARGET_INSTRUCTION(MOV32rm, "mov32rm", 11, { W, R })
TARGET_INSTRUCTION(MOV64rm, "mov64rm", 12, { W, R })

// STORE [mem], reg (Op0: Read base mem, Op1: Read src)
TARGET_INSTRUCTION(MOV8mr, "mov8mr", 13, { R, R })
TARGET_INSTRUCTION(MOV16mr, "mov16mr", 14, { R, R })
TARGET_INSTRUCTION(MOV32mr, "mov32mr", 15, { R, R })
TARGET_INSTRUCTION(MOV64mr, "mov64mr", 16, { R, R })

// Floating Point Movement
TARGET_INSTRUCTION(MOVSSrr, "movssrr", 17, { W, R })
TARGET_INSTRUCTION(MOVSSrm, "movssrm", 18, { W, R })
TARGET_INSTRUCTION(MOVSSmr, "movssmr", 19, { R, R })
TARGET_INSTRUCTION(MOVSDrr, "movsdrr", 20, { W, R })
TARGET_INSTRUCTION(MOVSDrm, "movsdrm", 21, { W, R })
TARGET_INSTRUCTION(MOVSDmr, "movsdmr", 22, { R, R })

// =========================================================================
// Integer ALU (2-Address Form: Op0 = Op0 op Op1)
// =========================================================================
TARGET_INSTRUCTION(ADD8rr, "add8rr", 23, { RW, R })
TARGET_INSTRUCTION(ADD16rr, "add16rr", 24, { RW, R })
TARGET_INSTRUCTION(ADD32rr, "add32rr", 25, { RW, R })
TARGET_INSTRUCTION(ADD64rr, "add64rr", 26, { RW, R })

TARGET_INSTRUCTION(SUB8rr, "sub8rr", 27, { RW, R })
TARGET_INSTRUCTION(SUB16rr, "sub16rr", 28, { RW, R })
TARGET_INSTRUCTION(SUB32rr, "sub32rr", 29, { RW, R })
TARGET_INSTRUCTION(SUB64rr, "sub64rr", 30, { RW, R })

TARGET_INSTRUCTION(AND8rr, "and8rr", 31, { RW, R })
TARGET_INSTRUCTION(AND16rr, "and16rr", 32, { RW, R })
TARGET_INSTRUCTION(AND32rr, "and32rr", 33, { RW, R })
TARGET_INSTRUCTION(AND64rr, "and64rr", 34, { RW, R })

TARGET_INSTRUCTION(OR8rr, "or8rr", 35, { RW, R })
TARGET_INSTRUCTION(OR16rr, "or16rr", 36, { RW, R })
TARGET_INSTRUCTION(OR32rr, "or32rr", 37, { RW, R })
TARGET_INSTRUCTION(OR64rr, "or64rr", 38, { RW, R })

TARGET_INSTRUCTION(XOR8rr, "xor8rr", 39, { RW, R })
TARGET_INSTRUCTION(XOR16rr, "xor16rr", 40, { RW, R })
TARGET_INSTRUCTION(XOR32rr, "xor32rr", 41, { RW, R })
TARGET_INSTRUCTION(XOR64rr, "xor64rr", 42, { RW, R })

// Comparison / Non-destructive tests (Op0: Read, Op1: Read)
TARGET_INSTRUCTION(CMP8rr, "cmp8rr", 43, { R, R })
TARGET_INSTRUCTION(CMP16rr, "cmp16rr", 44, { R, R })
TARGET_INSTRUCTION(CMP32rr, "cmp32rr", 45, { R, R })
TARGET_INSTRUCTION(CMP64rr, "cmp64rr", 46, { R, R })

TARGET_INSTRUCTION(TEST8rr, "test8rr", 47, { R, R })
TARGET_INSTRUCTION(TEST16rr, "test16rr", 48, { R, R })
TARGET_INSTRUCTION(TEST32rr, "test32rr", 49, { R, R })
TARGET_INSTRUCTION(TEST64rr, "test64rr", 50, { R, R })

TARGET_INSTRUCTION(ADC32rr, "adc32rr", 51, { RW, R })
TARGET_INSTRUCTION(ADC64rr, "adc64rr", 52, { RW, R })
TARGET_INSTRUCTION(SBB32rr, "sbb32rr", 53, { RW, R })
TARGET_INSTRUCTION(SBB64rr, "sbb64rr", 54, { RW, R })

// =========================================================================
// Wide Math
// =========================================================================
TARGET_INSTRUCTION(IMUL16rr, "imul16rr", 55, { RW, R })
TARGET_INSTRUCTION(IMUL32rr, "imul32rr", 56, { RW, R })
TARGET_INSTRUCTION(IMUL64rr, "imul64rr", 57, { RW, R })

// Single-operand unary divisions (Op0: Read divisor; dividend/quotient handled via implicit registers)
TARGET_INSTRUCTION(IDIV32r, "idiv32r", 58, { R })
TARGET_INSTRUCTION(IDIV64r, "idiv64r", 59, { R })
TARGET_INSTRUCTION(DIV32r, "div32r", 60, { R })
TARGET_INSTRUCTION(DIV64r, "div64r", 61, { R })

// =========================================================================
// Floating Point ALU
// =========================================================================
TARGET_INSTRUCTION(FADD32rr, "fadd32rr", 62, { RW, R })
TARGET_INSTRUCTION(FADD64rr, "fadd64rr", 63, { RW, R })
TARGET_INSTRUCTION(FSUB32rr, "fsub32rr", 64, { RW, R })
TARGET_INSTRUCTION(FSUB64rr, "fsub64rr", 65, { RW, R })
TARGET_INSTRUCTION(FMUL32rr, "fmul32rr", 66, { RW, R })
TARGET_INSTRUCTION(FMUL64rr, "fmul64rr", 67, { RW, R })
TARGET_INSTRUCTION(FDIV32rr, "fdiv32rr", 68, { RW, R })
TARGET_INSTRUCTION(FDIV64rr, "fdiv64rr", 69, { RW, R })
TARGET_INSTRUCTION(FCMP32rr, "fcmp32rr", 70, { R, R })
TARGET_INSTRUCTION(FCMP64rr, "fcmp64rr", 71, { R, R })

// =========================================================================
// Unary & Shifts
// =========================================================================
TARGET_INSTRUCTION(NEG32r, "neg32r", 72, { RW })
TARGET_INSTRUCTION(NEG64r, "neg64r", 73, { RW })
TARGET_INSTRUCTION(NOT32r, "not32r", 74, { RW })
TARGET_INSTRUCTION(NOT64r, "not64r", 75, { RW })

TARGET_INSTRUCTION(SHL32rr, "shl32rr", 76, { RW, R })
TARGET_INSTRUCTION(SHL64rr, "shl64rr", 77, { RW, R })
TARGET_INSTRUCTION(SHR32rr, "shr32rr", 78, { RW, R })
TARGET_INSTRUCTION(SHR64rr, "shr64rr", 79, { RW, R })
TARGET_INSTRUCTION(SAR32rr, "sar32rr", 80, { RW, R })
TARGET_INSTRUCTION(SAR64rr, "sar64rr", 81, { RW, R })

// =========================================================================
// Conversions & Casting (Op0: Write dst, Op1: Read src)
// =========================================================================
TARGET_INSTRUCTION(MOVZX32rr8, "movzx32rr8", 82, { W, R })
TARGET_INSTRUCTION(MOVZX32rr16, "movzx32rr16", 83, { W, R })
TARGET_INSTRUCTION(MOVZX64rr32, "movzx64rr32", 84, { W, R })
TARGET_INSTRUCTION(MOVSX32rr8, "movsx32rr8", 85, { W, R })
TARGET_INSTRUCTION(MOVSX32rr16, "movsx32rr16", 86, { W, R })
TARGET_INSTRUCTION(MOVSX64rr32, "movsx64rr32", 87, { W, R })
TARGET_INSTRUCTION(TRUNC8rr, "trunc8rr", 88, { W, R })
TARGET_INSTRUCTION(TRUNC16rr, "trunc16rr", 89, { W, R })
TARGET_INSTRUCTION(TRUNC32rr, "trunc32rr", 90, { W, R })
TARGET_INSTRUCTION(CVTSS2SDrr, "cvtss2sdrr", 91, { W, R })
TARGET_INSTRUCTION(CVTSD2SSrr, "cvtsd2ssrr", 92, { W, R })
TARGET_INSTRUCTION(CVTSI2SSrr, "cvtsi2ssrr", 93, { W, R })
TARGET_INSTRUCTION(CVTSI2SDrr, "cvtsi2sdrr", 94, { W, R })
TARGET_INSTRUCTION(CVTSS2SIrr, "cvtss2sirr", 95, { W, R })
TARGET_INSTRUCTION(CVTSD2SIrr, "cvtsd2sirr", 96, { W, R })

// =========================================================================
// Control Flow & Stack
// =========================================================================
TARGET_INSTRUCTION(JMP, "jmp", 97, { R })
TARGET_INSTRUCTION(JE, "je", 98, { R })
TARGET_INSTRUCTION(JNE, "jne", 99, { R })
TARGET_INSTRUCTION(JG, "jg", 100, { R })
TARGET_INSTRUCTION(JGE, "jge", 101, { R })
TARGET_INSTRUCTION(JL, "jl", 102, { R })
TARGET_INSTRUCTION(JLE, "jle", 103, { R })
TARGET_INSTRUCTION(JA, "ja", 104, { R })
TARGET_INSTRUCTION(JB, "jb", 105, { R })

TARGET_INSTRUCTION(PUSH64r, "push64r", 106, { R })
TARGET_INSTRUCTION(POP64r, "pop64r", 107, { W })
TARGET_INSTRUCTION(CALL, "call", 108, { R })
TARGET_INSTRUCTION(RET, "ret", 109, {})
TARGET_INSTRUCTION(NOP, "nop", 110, {})
TARGET_INSTRUCTION(HLT, "hlt", 111, {})
TARGET_INSTRUCTION(SYSCALL, "syscall", 112, {})

// =========================================================================
// Pseudo Instructions
// =========================================================================
// DYNAMIC_ALLOC dstPtr, sizeReg (Op0: Write allocated ptr, Op1: Read size)
TARGET_INSTRUCTION(DYNAMIC_ALLOC, "pseudo_dalloc", 200, { W, R })
// ALLOC dstPtr, staticSlot (Op0: Write allocated ptr, Op1: Read slot)
TARGET_INSTRUCTION(ALLOC, "pseudo_alloc", 201, { W, R })

#ifdef DEFINE_LOCAL_FLAGS
#undef R
#undef W
#undef RW
#undef DEFINE_LOCAL_FLAGS
#endif