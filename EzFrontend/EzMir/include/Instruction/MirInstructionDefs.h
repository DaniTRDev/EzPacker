/**
 * @file MirInstructionDefs.h
 * @brief Compile-time instruction catalogue: opcodes, flags, and metadata.
 *
 * This header turns the X-macro list in `MirInstructionSet.h` into the public
 * instruction definitions used throughout EzMir:
 *   - `MirInstructionFlags`: bit flags describing operand roles, control-flow,
 *     memory behavior, and side effects.
 *   - `MirInstructionOpCode`: the opcode enum.
 *   - `MirInstructionMetadata`: one metadata record per opcode.
 *   - `g_MirInstructionSet`: opcode-indexed metadata table.
 *   - `g_String2MirInstruction`: lowercase name -> opcode lookup map.
 *
 * Because the tables are generated from one source list, adding or changing an
 * instruction requires editing only `MirInstructionSet.h`.
 */
#ifndef EZPACKER_MIRINSTRUCTIONDEFS_H
#define EZPACKER_MIRINSTRUCTIONDEFS_H

#ifndef EZPACKER_HIGHLEVELMIRDEFS_H
#define EZPACKER_HIGHLEVELMIRDEFS_H

#include <cstdint>

/**
 * Bit flags attached to instruction metadata.
 *
 * Individual flags are combined to express the semantic contract of an opcode:
 * which operands are read or written, whether memory or CPU flags are touched,
 * whether the instruction terminates a block, and so on.
 *
 * MirReference acts as a MEMORY operand.
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
    Op1_MustBeRef = 1 << 4,
    Op1_MustBeReg = 1 << 5,
    Op1_MustBeImm = 1 << 6,
    Op1_MustBeMem = 1 << 7,
    Op2_MustBeReg = 1 << 8,
    Op2_MustBeMem = 1 << 9,
    Op2_MustBeImm = 1 << 10,

    // --- 3. Type & Size Safety ---
    SizeMatch = 1 << 11,   // Op1 and Op2 must be the exact same bit-width
    DestLarger = 1 << 12,  // sizeof(Op1) > sizeof(Op2) (e.g., ZEXT, SEXT)
    DestSmaller = 1 << 13, // sizeof(Op1) < sizeof(Op2) (e.g., TRUNC)

    // --- 4. Memory Semantics ---
    // Differentiates between instructions that calculate addresses (LEA) vs touch RAM (LOAD)
    ReadsMemory = 1 << 14,  // Reads from RAM
    WritesMemory = 1 << 15, // Writes to RAM

    // --- 5. Control Flow & Graph Building ---
    IsTerminator = 1 << 16, // Ends a Basic Block (JMP, RET, HALT)
    IsBranch = 1 << 17,     // Conditional Control Flow (JE, JNE)
    IsCall = 1 << 18,       // Function Call (Implies caller-saved registers are clobbered)
    IsReturn = 1 << 19,     // Returns from function

    // --- 6. Optimization Barriers ---
    HasSideEffect = 1 << 20, // Cannot be optimized away or reordered (SYSCALL, Volatile)

    // --- 7. CPU Status Flags (Implicit State) ---
    // Critical for preventing optimizations from breaking conditional jumps
    ReadsCPUFlags = 1 << 21,  // Depends on previous CMP/TEST (e.g., JE, CMOV)
    WritesCPUFlags = 1 << 22, // Overwrites CPU flags (e.g., ADD, CMP, AND)

    // ==========================================
    // --- Composite Shortcuts ---
    // Combine flags using bitwise OR for cleaner X-Macros
    // ==========================================

    // Operand combinations
    Op1_MustBeRegOrImm = 1 << 23,
    Op1_MustBeMemOrReg = 1 << 24,
    Op2_MustBeMemOrReg = 1 << 25,
    Op2_MustBeRegOrImm = 1 << 26,

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

/**
 * Returns the metadata entry for a given opcode.
 *
 * The opcode value is used directly as an index into `g_MirInstructionSet`.
 */
inline const MirInstructionMetadata &getMeta(MirInstructionOpCode op) { return g_MirInstructionSet[op]; }

/**
 * Resolves a textual opcode name to its enum value.
 *
 * Lookup is case-insensitive because both the input and generated table keys
 * are normalized with `StrToLower()`. If the string is unknown,
 * `MirInstructionOpCode::INVALID` is returned.
 */
inline MirInstructionOpCode getOpCodeFromStr(const std::string &str)
{
    auto it = g_String2MirInstruction.find(StrToLower(str));

    if (it == g_String2MirInstruction.end())
        return MirInstructionOpCode::INVALID;

    return it->second;
}

#endif // EZPACKER_HIGHLEVELMIRDEFS_H

#endif // EZPACKER_MIRINSTRUCTIONDEFS_H
