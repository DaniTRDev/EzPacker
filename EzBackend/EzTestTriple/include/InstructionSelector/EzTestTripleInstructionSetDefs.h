#ifndef R
#define R OperandFlag::Read
#define W OperandFlag::Write
#define RW (OperandFlag::Read | OperandFlag::Write)
#define DEFINE_LOCAL_FLAGS
#endif

// =========================================================================
// Data Movement & Addressing
// =========================================================================
// Register-to-Register Moves
TARGET_INSTRUCTION(MOV8rr, "mov8rr", 1, { W, R })
TARGET_INSTRUCTION(MOV16rr, "mov16rr", 2, { W, R })
TARGET_INSTRUCTION(MOV32rr, "mov32rr", 3, { W, R })
TARGET_INSTRUCTION(MOV64rr, "mov64rr", 4, { W, R })
TARGET_INSTRUCTION(MOVSSrr, "movssrr", 5, { W, R })
TARGET_INSTRUCTION(MOVSDrr, "movsdrr", 6, { W, R })

// Immediate-to-Register Moves
TARGET_INSTRUCTION(MOV8ri, "mov8ri", 7, { W, R })
TARGET_INSTRUCTION(MOV16ri, "mov16ri", 8, { W, R })
TARGET_INSTRUCTION(MOV32ri, "mov32ri", 9, { W, R })
TARGET_INSTRUCTION(MOV64ri32, "mov64ri32", 10, { W, R })   // Sign-extended 32-bit immediate
TARGET_INSTRUCTION(MOVABS64ri, "movabs64ri", 11, { W, R }) // Full 64-bit immediate / symbol address

// Load Effective Address (LEA)
TARGET_INSTRUCTION(LEA64rm, "lea64rm", 12, { W, R })         // Base + index*scale + disp
TARGET_INSTRUCTION(LEA64rm_rip, "lea64rm_rip", 13, { W, R }) // RIP-relative address materialization

// Direct / Base-Indirect Loads
TARGET_INSTRUCTION(MOV8rm, "mov8rm", 14, { W, R })
TARGET_INSTRUCTION(MOV16rm, "mov16rm", 15, { W, R })
TARGET_INSTRUCTION(MOV32rm, "mov32rm", 16, { W, R })
TARGET_INSTRUCTION(MOV64rm, "mov64rm", 17, { W, R })
TARGET_INSTRUCTION(MOVSSrm, "movssrm", 18, { W, R })
TARGET_INSTRUCTION(MOVSDrm, "movsdrm", 19, { W, R })

// RIP-Relative Loads (Small Code Model)
TARGET_INSTRUCTION(MOV8rm_rip, "mov8rm_rip", 20, { W, R })
TARGET_INSTRUCTION(MOV16rm_rip, "mov16rm_rip", 21, { W, R })
TARGET_INSTRUCTION(MOV32rm_rip, "mov32rm_rip", 22, { W, R })
TARGET_INSTRUCTION(MOV64rm_rip, "mov64rm_rip", 23, { W, R })
TARGET_INSTRUCTION(MOVSSrm_rip, "movssrm_rip", 24, { W, R })
TARGET_INSTRUCTION(MOVSDrm_rip, "movsdrm_rip", 25, { W, R })

// Direct / Base-Indirect Stores (Register Source)
TARGET_INSTRUCTION(MOV8mr, "mov8mr", 26, { R, R })
TARGET_INSTRUCTION(MOV16mr, "mov16mr", 27, { R, R })
TARGET_INSTRUCTION(MOV32mr, "mov32mr", 28, { R, R })
TARGET_INSTRUCTION(MOV64mr, "mov64mr", 29, { R, R })
TARGET_INSTRUCTION(MOVSSmr, "movssmr", 30, { R, R })
TARGET_INSTRUCTION(MOVSDmr, "movsdmr", 31, { R, R })

// Direct / Base-Indirect Stores (Immediate Source)
TARGET_INSTRUCTION(MOV8mi, "mov8mi", 32, { R, R })
TARGET_INSTRUCTION(MOV16mi, "mov16mi", 33, { R, R })
TARGET_INSTRUCTION(MOV32mi, "mov32mi", 34, { R, R })
TARGET_INSTRUCTION(MOV64mi32, "mov64mi32", 35, { R, R })

// RIP-Relative Stores (Small Code Model)
TARGET_INSTRUCTION(MOV8mr_rip, "mov8mr_rip", 36, { R, R })
TARGET_INSTRUCTION(MOV16mr_rip, "mov16mr_rip", 37, { R, R })
TARGET_INSTRUCTION(MOV32mr_rip, "mov32mr_rip", 38, { R, R })
TARGET_INSTRUCTION(MOV64mr_rip, "mov64mr_rip", 39, { R, R })
TARGET_INSTRUCTION(MOVSSmr_rip, "movssmr_rip", 40, { R, R })
TARGET_INSTRUCTION(MOVSDmr_rip, "movsdmr_rip", 41, { R, R })

// =========================================================================
// Integer Arithmetic & Logic (2-Address Form: Op0 = Op0 op Op1)
// =========================================================================
// ADD
TARGET_INSTRUCTION(ADD8rr, "add8rr", 42, { RW, R })
TARGET_INSTRUCTION(ADD16rr, "add16rr", 43, { RW, R })
TARGET_INSTRUCTION(ADD32rr, "add32rr", 44, { RW, R })
TARGET_INSTRUCTION(ADD64rr, "add64rr", 45, { RW, R })
TARGET_INSTRUCTION(ADD8ri, "add8ri", 46, { RW, R })
TARGET_INSTRUCTION(ADD16ri, "add16ri", 47, { RW, R })
TARGET_INSTRUCTION(ADD32ri, "add32ri", 48, { RW, R })
TARGET_INSTRUCTION(ADD64ri32, "add64ri32", 49, { RW, R })

// ADC (Add with Carry)
TARGET_INSTRUCTION(ADC8rr, "adc8rr", 50, { RW, R })
TARGET_INSTRUCTION(ADC16rr, "adc16rr", 51, { RW, R })
TARGET_INSTRUCTION(ADC32rr, "adc32rr", 52, { RW, R })
TARGET_INSTRUCTION(ADC64rr, "adc64rr", 53, { RW, R })
TARGET_INSTRUCTION(ADC8ri, "adc8ri", 54, { RW, R })
TARGET_INSTRUCTION(ADC16ri, "adc16ri", 55, { RW, R })
TARGET_INSTRUCTION(ADC32ri, "adc32ri", 56, { RW, R })
TARGET_INSTRUCTION(ADC64ri32, "adc64ri32", 57, { RW, R })

// SUB
TARGET_INSTRUCTION(SUB8rr, "sub8rr", 58, { RW, R })
TARGET_INSTRUCTION(SUB16rr, "sub16rr", 59, { RW, R })
TARGET_INSTRUCTION(SUB32rr, "sub32rr", 60, { RW, R })
TARGET_INSTRUCTION(SUB64rr, "sub64rr", 61, { RW, R })
TARGET_INSTRUCTION(SUB8ri, "sub8ri", 62, { RW, R })
TARGET_INSTRUCTION(SUB16ri, "sub16ri", 63, { RW, R })
TARGET_INSTRUCTION(SUB32ri, "sub32ri", 64, { RW, R })
TARGET_INSTRUCTION(SUB64ri32, "sub64ri32", 65, { RW, R })

// SBB (Subtract with Borrow)
TARGET_INSTRUCTION(SBB8rr, "sbb8rr", 66, { RW, R })
TARGET_INSTRUCTION(SBB16rr, "sbb16rr", 67, { RW, R })
TARGET_INSTRUCTION(SBB32rr, "sbb32rr", 68, { RW, R })
TARGET_INSTRUCTION(SBB64rr, "sbb64rr", 69, { RW, R })
TARGET_INSTRUCTION(SBB8ri, "sbb8ri", 70, { RW, R })
TARGET_INSTRUCTION(SBB16ri, "sbb16ri", 71, { RW, R })
TARGET_INSTRUCTION(SBB32ri, "sbb32ri", 72, { RW, R })
TARGET_INSTRUCTION(SBB64ri32, "sbb64ri32", 73, { RW, R })

// Bitwise AND
TARGET_INSTRUCTION(AND8rr, "and8rr", 74, { RW, R })
TARGET_INSTRUCTION(AND16rr, "and16rr", 75, { RW, R })
TARGET_INSTRUCTION(AND32rr, "and32rr", 76, { RW, R })
TARGET_INSTRUCTION(AND64rr, "and64rr", 77, { RW, R })
TARGET_INSTRUCTION(AND8ri, "and8ri", 78, { RW, R })
TARGET_INSTRUCTION(AND16ri, "and16ri", 79, { RW, R })
TARGET_INSTRUCTION(AND32ri, "and32ri", 80, { RW, R })
TARGET_INSTRUCTION(AND64ri32, "and64ri32", 81, { RW, R })

// Bitwise OR
TARGET_INSTRUCTION(OR8rr, "or8rr", 82, { RW, R })
TARGET_INSTRUCTION(OR16rr, "or16rr", 83, { RW, R })
TARGET_INSTRUCTION(OR32rr, "or32rr", 84, { RW, R })
TARGET_INSTRUCTION(OR64rr, "or64rr", 85, { RW, R })
TARGET_INSTRUCTION(OR8ri, "or8ri", 86, { RW, R })
TARGET_INSTRUCTION(OR16ri, "or16ri", 87, { RW, R })
TARGET_INSTRUCTION(OR32ri, "or32ri", 88, { RW, R })
TARGET_INSTRUCTION(OR64ri32, "or64ri32", 89, { RW, R })

// Bitwise XOR
TARGET_INSTRUCTION(XOR8rr, "xor8rr", 90, { RW, R })
TARGET_INSTRUCTION(XOR16rr, "xor16rr", 91, { RW, R })
TARGET_INSTRUCTION(XOR32rr, "xor32rr", 92, { RW, R })
TARGET_INSTRUCTION(XOR64rr, "xor64rr", 93, { RW, R })
TARGET_INSTRUCTION(XOR8ri, "xor8ri", 94, { RW, R })
TARGET_INSTRUCTION(XOR16ri, "xor16ri", 95, { RW, R })
TARGET_INSTRUCTION(XOR32ri, "xor32ri", 96, { RW, R })
TARGET_INSTRUCTION(XOR64ri32, "xor64ri32", 97, { RW, R })

// Integer Comparisons
TARGET_INSTRUCTION(CMP8rr, "cmp8rr", 98, { R, R })
TARGET_INSTRUCTION(CMP16rr, "cmp16rr", 99, { R, R })
TARGET_INSTRUCTION(CMP32rr, "cmp32rr", 100, { R, R })
TARGET_INSTRUCTION(CMP64rr, "cmp64rr", 101, { R, R })
TARGET_INSTRUCTION(CMP8ri, "cmp8ri", 102, { R, R })
TARGET_INSTRUCTION(CMP16ri, "cmp16ri", 103, { R, R })
TARGET_INSTRUCTION(CMP32ri, "cmp32ri", 104, { R, R })
TARGET_INSTRUCTION(CMP64ri32, "cmp64ri32", 105, { R, R })

// Non-destructive Tests
TARGET_INSTRUCTION(TEST8rr, "test8rr", 106, { R, R })
TARGET_INSTRUCTION(TEST16rr, "test16rr", 107, { R, R })
TARGET_INSTRUCTION(TEST32rr, "test32rr", 108, { R, R })
TARGET_INSTRUCTION(TEST64rr, "test64rr", 109, { R, R })
TARGET_INSTRUCTION(TEST8ri, "test8ri", 110, { R, R })
TARGET_INSTRUCTION(TEST16ri, "test16ri", 111, { R, R })
TARGET_INSTRUCTION(TEST32ri, "test32ri", 112, { R, R })
TARGET_INSTRUCTION(TEST64ri32, "test64ri32", 113, { R, R })

// Multiplications
TARGET_INSTRUCTION(MUL8r, "mul8r", 114, { RW, R })
TARGET_INSTRUCTION(MUL16r, "mul16r", 115, { RW, R })
TARGET_INSTRUCTION(MUL32r, "mul32r", 116, { RW, R })
TARGET_INSTRUCTION(MUL64r, "mul64r", 117, { RW, R })
TARGET_INSTRUCTION(IMUL8r, "imul8r", 118, { RW, R })
TARGET_INSTRUCTION(IMUL16rr, "imul16rr", 119, { RW, R })
TARGET_INSTRUCTION(IMUL32rr, "imul32rr", 120, { RW, R })
TARGET_INSTRUCTION(IMUL64rr, "imul64rr", 121, { RW, R })
TARGET_INSTRUCTION(IMUL16rri, "imul16rri", 122, { W, R, R })
TARGET_INSTRUCTION(IMUL32rri, "imul32rri", 123, { W, R, R })
TARGET_INSTRUCTION(IMUL64rri32, "imul64rri32", 124, { W, R, R })

// Divisions & Remainders
TARGET_INSTRUCTION(DIV8r, "div8r", 125, { RW, R })
TARGET_INSTRUCTION(DIV16r, "div16r", 126, { RW, R })
TARGET_INSTRUCTION(DIV32r, "div32r", 127, { RW, R })
TARGET_INSTRUCTION(DIV64r, "div64r", 128, { RW, R })
TARGET_INSTRUCTION(IDIV8r, "idiv8r", 129, { RW, R })
TARGET_INSTRUCTION(IDIV16r, "idiv16r", 130, { RW, R })
TARGET_INSTRUCTION(IDIV32r, "idiv32r", 131, { RW, R })
TARGET_INSTRUCTION(IDIV64r, "idiv64r", 132, { RW, R })

// Unary Operations
TARGET_INSTRUCTION(NEG8r, "neg8r", 133, { RW })
TARGET_INSTRUCTION(NEG16r, "neg16r", 134, { RW })
TARGET_INSTRUCTION(NEG32r, "neg32r", 135, { RW })
TARGET_INSTRUCTION(NEG64r, "neg64r", 136, { RW })
TARGET_INSTRUCTION(NOT8r, "not8r", 137, { RW })
TARGET_INSTRUCTION(NOT16r, "not16r", 138, { RW })
TARGET_INSTRUCTION(NOT32r, "not32r", 139, { RW })
TARGET_INSTRUCTION(NOT64r, "not64r", 140, { RW })

// Shifts (Reg-Reg and Reg-Imm)
TARGET_INSTRUCTION(SHL8rr, "shl8rr", 141, { RW, R })
TARGET_INSTRUCTION(SHL16rr, "shl16rr", 142, { RW, R })
TARGET_INSTRUCTION(SHL32rr, "shl32rr", 143, { RW, R })
TARGET_INSTRUCTION(SHL64rr, "shl64rr", 144, { RW, R })
TARGET_INSTRUCTION(SHL8ri, "shl8ri", 145, { RW, R })
TARGET_INSTRUCTION(SHL16ri, "shl16ri", 146, { RW, R })
TARGET_INSTRUCTION(SHL32ri, "shl32ri", 147, { RW, R })
TARGET_INSTRUCTION(SHL64ri, "shl64ri", 148, { RW, R })

TARGET_INSTRUCTION(SHR8rr, "shr8rr", 149, { RW, R })
TARGET_INSTRUCTION(SHR16rr, "shr16rr", 150, { RW, R })
TARGET_INSTRUCTION(SHR32rr, "shr32rr", 151, { RW, R })
TARGET_INSTRUCTION(SHR64rr, "shr64rr", 152, { RW, R })
TARGET_INSTRUCTION(SHR8ri, "shr8ri", 153, { RW, R })
TARGET_INSTRUCTION(SHR16ri, "shr16ri", 154, { RW, R })
TARGET_INSTRUCTION(SHR32ri, "shr32ri", 155, { RW, R })
TARGET_INSTRUCTION(SHR64ri, "shr64ri", 156, { RW, R })

TARGET_INSTRUCTION(SAR8rr, "sar8rr", 157, { RW, R })
TARGET_INSTRUCTION(SAR16rr, "sar16rr", 158, { RW, R })
TARGET_INSTRUCTION(SAR32rr, "sar32rr", 159, { RW, R })
TARGET_INSTRUCTION(SAR64rr, "sar64rr", 160, { RW, R })
TARGET_INSTRUCTION(SAR8ri, "sar8ri", 161, { RW, R })
TARGET_INSTRUCTION(SAR16ri, "sar16ri", 162, { RW, R })
TARGET_INSTRUCTION(SAR32ri, "sar32ri", 163, { RW, R })
TARGET_INSTRUCTION(SAR64ri, "sar64ri", 164, { RW, R })

// =========================================================================
// Floating Point ALU (SSE / Scalar FP)
// =========================================================================
TARGET_INSTRUCTION(FADD32rr, "fadd32rr", 165, { RW, R })
TARGET_INSTRUCTION(FADD64rr, "fadd64rr", 166, { RW, R })
TARGET_INSTRUCTION(FSUB32rr, "fsub32rr", 167, { RW, R })
TARGET_INSTRUCTION(FSUB64rr, "fsub64rr", 168, { RW, R })
TARGET_INSTRUCTION(FMUL32rr, "fmul32rr", 169, { RW, R })
TARGET_INSTRUCTION(FMUL64rr, "fmul64rr", 170, { RW, R })
TARGET_INSTRUCTION(FDIV32rr, "fdiv32rr", 171, { RW, R })
TARGET_INSTRUCTION(FDIV64rr, "fdiv64rr", 172, { RW, R })
TARGET_INSTRUCTION(FCMP32rr, "fcmp32rr", 173, { R, R })
TARGET_INSTRUCTION(FCMP64rr, "fcmp64rr", 174, { R, R })

// =========================================================================
// Conversions, Extensions & Truncations
// =========================================================================
TARGET_INSTRUCTION(MOVZX16rr8, "movzx16rr8", 175, { W, R })
TARGET_INSTRUCTION(MOVZX32rr8, "movzx32rr8", 176, { W, R })
TARGET_INSTRUCTION(MOVZX32rr16, "movzx32rr16", 177, { W, R })
TARGET_INSTRUCTION(MOVZX64rr8, "movzx64rr8", 178, { W, R })
TARGET_INSTRUCTION(MOVZX64rr16, "movzx64rr16", 179, { W, R })
TARGET_INSTRUCTION(MOVZX64rr32, "movzx64rr32", 180, { W, R })

TARGET_INSTRUCTION(MOVSX16rr8, "movsx16rr8", 181, { W, R })
TARGET_INSTRUCTION(MOVSX32rr8, "movsx32rr8", 182, { W, R })
TARGET_INSTRUCTION(MOVSX32rr16, "movsx32rr16", 183, { W, R })
TARGET_INSTRUCTION(MOVSX64rr8, "movsx64rr8", 184, { W, R })
TARGET_INSTRUCTION(MOVSX64rr16, "movsx64rr16", 185, { W, R })
TARGET_INSTRUCTION(MOVSX64rr32, "movsx64rr32", 186, { W, R })

TARGET_INSTRUCTION(TRUNC8rr, "trunc8rr", 187, { W, R })
TARGET_INSTRUCTION(TRUNC16rr, "trunc16rr", 188, { W, R })
TARGET_INSTRUCTION(TRUNC32rr, "trunc32rr", 189, { W, R })

TARGET_INSTRUCTION(CVTSS2SDrr, "cvtss2sdrr", 190, { W, R })
TARGET_INSTRUCTION(CVTSD2SSrr, "cvtsd2ssrr", 191, { W, R })
TARGET_INSTRUCTION(CVTSI2SSrr32, "cvtsi2ssrr32", 192, { W, R })
TARGET_INSTRUCTION(CVTSI2SSrr64, "cvtsi2ssrr64", 193, { W, R })
TARGET_INSTRUCTION(CVTSI2SDrr32, "cvtsi2sdrr32", 194, { W, R })
TARGET_INSTRUCTION(CVTSI2SDrr64, "cvtsi2sdrr64", 195, { W, R })
TARGET_INSTRUCTION(CVTTSS2SIrr32, "cvttss2sirr32", 196, { W, R })
TARGET_INSTRUCTION(CVTTSS2SIrr64, "cvttss2sirr64", 197, { W, R })
TARGET_INSTRUCTION(CVTTSD2SIrr32, "cvttsd2sirr32", 198, { W, R })
TARGET_INSTRUCTION(CVTTSD2SIrr64, "cvttsd2sirr64", 199, { W, R })

// Bitcasts between GPR and FPR
TARGET_INSTRUCTION(MOVDtoFPR, "movd_to_fpr", 200, { W, R })
TARGET_INSTRUCTION(MOVDtoGPR, "movd_to_gpr", 201, { W, R })
TARGET_INSTRUCTION(MOVQtoFPR, "movq_to_fpr", 202, { W, R })
TARGET_INSTRUCTION(MOVQtoGPR, "movq_to_gpr", 203, { W, R })

// =========================================================================
// Control Flow & Subroutines
// =========================================================================
TARGET_INSTRUCTION(JMP, "jmp", 204, { R })
TARGET_INSTRUCTION(JE, "je", 205, { R })
TARGET_INSTRUCTION(JNE, "jne", 206, { R })
TARGET_INSTRUCTION(JG, "jg", 207, { R })
TARGET_INSTRUCTION(JGE, "jge", 208, { R })
TARGET_INSTRUCTION(JL, "jl", 209, { R })
TARGET_INSTRUCTION(JLE, "jle", 210, { R })
TARGET_INSTRUCTION(JA, "ja", 211, { R })
TARGET_INSTRUCTION(JB, "jb", 212, { R })

TARGET_INSTRUCTION(JMP64r, "jmp64r", 213, { R })
TARGET_INSTRUCTION(CALL64r, "call64r", 214, { R })
TARGET_INSTRUCTION(CALL, "call", 215, { R })
TARGET_INSTRUCTION(RET, "ret", 216, {})

// Stack Operations
TARGET_INSTRUCTION(PUSH64r, "push64r", 217, { R })
TARGET_INSTRUCTION(PUSH64i32, "push64i32", 218, { R })
TARGET_INSTRUCTION(POP64r, "pop64r", 219, { W })

// System & Process Ops
TARGET_INSTRUCTION(NOP, "nop", 220, {})
TARGET_INSTRUCTION(HLT, "hlt", 221, {})
TARGET_INSTRUCTION(SYSCALL, "syscall", 222, {})

// =========================================================================
// Pseudo Instructions
// =========================================================================
TARGET_INSTRUCTION(ALLOC, "pseudo_alloc", 250, { W, R })
TARGET_INSTRUCTION(DYNAMIC_ALLOC, "pseudo_dynamic_alloc", 251, { W, R })

#ifdef DEFINE_LOCAL_FLAGS
#undef R
#undef W
#undef RW
#undef DEFINE_LOCAL_FLAGS
#endif