#ifndef EZPACKER_MIRINSTRUCTIONDEFS_H
#define EZPACKER_MIRINSTRUCTIONDEFS_H

#ifndef EZPACKER_HIGHLEVELMIRDEFS_H
#define EZPACKER_HIGHLEVELMIRDEFS_H

#include <cstdint>

/**
 * File used to define utility to be able to define the instruction set at compile time with easy "command-like"
 * methods.
 */

enum MirInstructionFlags : uint32_t
{
    None = 0,

    // --- 1. Data Flow (Liveness Analysis) ---
    // Defines how operands are accessed. Critical for Register Allocation.
    Op1_Read = 1 << 0,  // Operand 1 is read
    Op1_Write = 1 << 1, // Operand 1 is written/defined
    Op2_Read = 1 << 2,  // Operand 2 is read
    Op2_Write = 1 << 3, // Operand 2 is written (e.g., XCHG instructions)

    // --- 2. Operand Constraints ---
    // Strict enforcement of what the operand can physically be.
    Op1_MustBeReg = 1 << 4,
    Op1_MustBeMem = 1 << 5,
    Op2_MustBeReg = 1 << 6,
    Op2_MustBeMem = 1 << 7,
    Op2_MustBeImm = 1 << 8,

    // --- 3. Type & Size Safety ---
    SizeMatch = 1 << 9,    // Op1 and Op2 must be the exact same bit-width
    DestLarger = 1 << 10,  // sizeof(Op1) > sizeof(Op2) (e.g., ZEXT, SEXT)
    DestSmaller = 1 << 11, // sizeof(Op1) < sizeof(Op2) (e.g., TRUNC)

    // --- 4. Memory Semantics ---
    // Differentiates between instructions that calculate addresses (LEA) vs touch RAM (LOAD)
    ReadsMemory = 1 << 12,  // Reads from RAM
    WritesMemory = 1 << 13, // Writes to RAM

    // --- 5. Control Flow & Graph Building ---
    IsTerminator = 1 << 14, // Ends a Basic Block (JMP, RET, HALT)
    IsBranch = 1 << 15,     // Conditional Control Flow (JE, JNE)
    IsCall = 1 << 16,       // Function Call (Implies caller-saved registers are clobbered)
    IsReturn = 1 << 17,     // Returns from function

    // --- 6. Optimization Barriers ---
    HasSideEffect = 1 << 18, // Cannot be optimized away or reordered (SYSCALL, Volatile)

    // --- 7. CPU Status Flags (Implicit State) ---
    // Critical for preventing optimizations from breaking conditional jumps
    ReadsCPUFlags = 1 << 19,  // Depends on previous CMP/TEST (e.g., JE, CMOV)
    WritesCPUFlags = 1 << 20, // Overwrites CPU flags (e.g., ADD, CMP, AND)

    // ==========================================
    // --- Composite Shortcuts ---
    // Combine flags using bitwise OR for cleaner X-Macros
    // ==========================================

    // Operand combinations
    Op1_MustBeMemOrReg = Op1_MustBeMem | Op1_MustBeReg,
    Op2_MustBeMemOrReg = Op2_MustBeMem | Op2_MustBeReg,
    Op2_MustBeRegOrImm = Op1_MustBeReg | Op2_MustBeImm,

    // Standard Math (e.g., ADD x, y -> x = x + y)
    ReadWrite = Op1_Read | Op1_Write | Op2_Read,

    // Load: Reads memory into a register (e.g., LOAD reg, [mem])
    IsLoad = Op1_Write | Op1_MustBeReg | Op2_Read | Op2_MustBeMem | ReadsMemory,

    // Store: Writes a register/imm to memory (e.g., STORE [mem], reg)
    IsStore = Op1_Read | Op1_MustBeMem | Op2_Read | WritesMemory | HasSideEffect
};

enum MirInstructionOpCode : uint8_t
{
#define INSTRUCTION(name, operandCount, flags) name,
#include "MirInstructionSet.h"
#undef INSTRUCTION
};

struct MirInstructionMetadata
{
    MirInstructionOpCode m_opcode;
    int8_t m_operandCount;
    uint32_t m_flags;
    std::string m_name;
};

inline const MirInstructionMetadata g_MirInstructionSet[] = {
#define INSTRUCTION(name, operandCount, flags)                                                                         \
    MirInstructionMetadata{ .m_opcode = name,                                                                          \
                            .m_operandCount = static_cast<int8_t>(operandCount),                                       \
                            .m_flags = flags,                                                                          \
                            .m_name = StrToLower(#name) },
#include "MirInstructionSet.h"
#undef INSTRUCTION
};

inline std::map<std::string, MirInstructionOpCode> g_String2MirInstruction = {
#define INSTRUCTION(name, operandCount, flags) { StrToLower(#name), name },
#include "MirInstructionSet.h"
#undef INSTRUCTION
};

inline const MirInstructionMetadata &getMeta(MirInstructionOpCode op) { return g_MirInstructionSet[op]; }

inline MirInstructionOpCode getOpCodeFromStr(const std::string &str)
{
    auto it = g_String2MirInstruction.find(StrToLower(str));

    if (it == g_String2MirInstruction.end())
        return MirInstructionOpCode::INVALID;

    return it->second;
}

#endif // EZPACKER_HIGHLEVELMIRDEFS_H

#endif // EZPACKER_MIRINSTRUCTIONDEFS_H
