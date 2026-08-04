#ifndef EZPACKER_EZTRIPLETESTINSTRUCTIONSET_H
#define EZPACKER_EZTRIPLETESTINSTRUCTIONSET_H

#include "EzTriple.h"

namespace EzTripleTestInstructionSet::TargetInst
{
constexpr MirTargetInstructionId MOV8rr = 1;
constexpr MirTargetInstructionId MOV16rr = 2;
constexpr MirTargetInstructionId MOV32rr = 3;
constexpr MirTargetInstructionId MOV64rr = 4;
constexpr MirTargetInstructionId MOVSSrr = 5; // Float 32
constexpr MirTargetInstructionId MOVSDrr = 6; // Float 64

// Loads (Memory to Register)
constexpr MirTargetInstructionId MOV8rm = 10;
constexpr MirTargetInstructionId MOV16rm = 11;
constexpr MirTargetInstructionId MOV32rm = 12;
constexpr MirTargetInstructionId MOV64rm = 13;
constexpr MirTargetInstructionId MOVSSrm = 14;
constexpr MirTargetInstructionId MOVSDrm = 15;

// Stores (Register to Memory)
constexpr MirTargetInstructionId MOV8mr = 20;
constexpr MirTargetInstructionId MOV16mr = 21;
constexpr MirTargetInstructionId MOV32mr = 22;
constexpr MirTargetInstructionId MOV64mr = 23;
constexpr MirTargetInstructionId MOVSSmr = 24;
constexpr MirTargetInstructionId MOVSDmr = 25;

// Memory Allocation / Address Generation
constexpr MirTargetInstructionId LEA64r = 30;

// Stack Arguments & Returns
constexpr MirTargetInstructionId PUSH64r = 31;
constexpr MirTargetInstructionId POP64r = 32;

// =========================================================================
// 2. ARITHMETIC & LOGIC (ALU)
// =========================================================================
// Addition
constexpr MirTargetInstructionId ADD8rr = 40;
constexpr MirTargetInstructionId ADD16rr = 41;
constexpr MirTargetInstructionId ADD32rr = 42;
constexpr MirTargetInstructionId ADD64rr = 43;

// Subtraction
constexpr MirTargetInstructionId SUB8rr = 50;
constexpr MirTargetInstructionId SUB16rr = 51;
constexpr MirTargetInstructionId SUB32rr = 52;
constexpr MirTargetInstructionId SUB64rr = 53;

// Carry Addition / Subtraction
constexpr MirTargetInstructionId ADC32rr = 60;
constexpr MirTargetInstructionId ADC64rr = 61;
constexpr MirTargetInstructionId SBB32rr = 62;
constexpr MirTargetInstructionId SBB64rr = 63;

// Multiplication & Division
constexpr MirTargetInstructionId IMUL16rr = 70;
constexpr MirTargetInstructionId IMUL32rr = 71;
constexpr MirTargetInstructionId IMUL64rr = 72;
constexpr MirTargetInstructionId DIV32r = 73;
constexpr MirTargetInstructionId DIV64r = 74;
constexpr MirTargetInstructionId IDIV32r = 75;
constexpr MirTargetInstructionId IDIV64r = 76;

// Bitwise Logic
constexpr MirTargetInstructionId AND32rr = 80;
constexpr MirTargetInstructionId AND64rr = 81;
constexpr MirTargetInstructionId OR32rr = 82;
constexpr MirTargetInstructionId OR64rr = 83;
constexpr MirTargetInstructionId XOR32rr = 84;
constexpr MirTargetInstructionId XOR64rr = 85;

// Comparisons & Tests
constexpr MirTargetInstructionId CMP32rr = 90;
constexpr MirTargetInstructionId CMP64rr = 91;
constexpr MirTargetInstructionId TEST32rr = 92;
constexpr MirTargetInstructionId TEST64rr = 93;

// Unary Operations
constexpr MirTargetInstructionId NEG32r = 100;
constexpr MirTargetInstructionId NEG64r = 101;
constexpr MirTargetInstructionId NOT32r = 102;
constexpr MirTargetInstructionId NOT64r = 103;

// Bit Shifts
constexpr MirTargetInstructionId SHL32rCL = 110;
constexpr MirTargetInstructionId SHR32rCL = 111;
constexpr MirTargetInstructionId SAR32rCL = 112;

// =========================================================================
// 3. CASTING & CONVERSIONS
// =========================================================================
constexpr MirTargetInstructionId MOVZX32rr8 = 120;
constexpr MirTargetInstructionId MOVSX32rr8 = 121;
constexpr MirTargetInstructionId TRUNC8rr = 122;
constexpr MirTargetInstructionId CVTSS2SDrr = 123;
constexpr MirTargetInstructionId MOVDQUrr = 124;

// =========================================================================
// 4. CONTROL FLOW & SYSTEM
// =========================================================================
// Branches
constexpr MirTargetInstructionId JMP = 130;
constexpr MirTargetInstructionId JE = 131;
constexpr MirTargetInstructionId JNE = 132;
constexpr MirTargetInstructionId JG = 133;
constexpr MirTargetInstructionId JGE = 134;
constexpr MirTargetInstructionId JL = 135;
constexpr MirTargetInstructionId JLE = 136;
constexpr MirTargetInstructionId JA = 137;
constexpr MirTargetInstructionId JB = 138;

// Call / Return
constexpr MirTargetInstructionId CALL = 140;
constexpr MirTargetInstructionId RET = 141;

// System Instructions
constexpr MirTargetInstructionId NOP = 150;
constexpr MirTargetInstructionId HLT = 151;
constexpr MirTargetInstructionId SYSCALL = 152;
} // namespace EzTripleTestInstructionSet::TargetInst

#endif // EZPACKER_EZTRIPLETESTINSTRUCTIONSET_H
