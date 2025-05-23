#ifndef EZPACKER_IRTYPES_H
#define EZPACKER_IRTYPES_H

#include "EzFrontendCommon.h"

#define UNSUPPORTED_IRINSTRUCTION_TYPE(Type) Type = IRInstructionType::Unsupported

/**
 * Enum that contains the very basic instructions that are defined in our IR.
 */
enum class IRInstructionType : uint8_t
{
    Invalid = 0,                               // Invalid or unrecognized instruction
    Unsupported = 1,                           // Used to mark certain instructions as unsupported.
    Add,                                       // Addition
    And,                                       // Bitwise AND
    Branch,                                    // Conditional branch (e.g., JE, BNE)
    Call,                                      // Function call
    Compare,                                   // Comparison instruction
    Divide,                                    // Division
    Exchange,                                  // Exchange 2 operands.
    FreeStack,                                 // Frees a portion of the stack.
    Jump,                                      // Unconditional jump
    Load,                                      // Loads memory into a register.
    LoadEffectiveAddress,                      // Compute and load effective address (e.g., LEA)
    Multiply,                                  // Multiplication
    Nop,                                       // No operation
    Not,                                       // Bitwise NOT
    Or,                                        // Bitwise OR
    Pop,                                       // Pop from stack
    UNSUPPORTED_IRINSTRUCTION_TYPE(Prefetch),  // Memory prefetch. Adds given block to CACHE (L1, L2, L3).
    Push,                                      // Push to stack
    ReserveStack,                              // Reserves a Number of bytes in the stack.
    Return,                                    // Function return
    RotateLeft,                                // Bitwise rotate left
    RotateRight,                               // Bitwise rotate right
    SetFlags,                                  // Set or clear flags
    ShiftLeft,                                 // Bitwise shift left
    ShiftRight,                                // Bitwise shift right
    SignExtend,                                // Sign extension
    Store,                                     // Loads a register with memory.
    Subtract,                                  // Subtraction
    UNSUPPORTED_IRINSTRUCTION_TYPE(SysCall),   // System call or interrupt
    Test,                                      // Bitwise test
    Xor,                                       // Bitwise XOR
    UNSUPPORTED_IRINSTRUCTION_TYPE(ZeroExtend) // Zero extension, specific of ARM.
};

extern const std::map<std::string_view, IRInstructionType> g_IRInstructionStr2Type;
extern const std::map<IRInstructionType, std::string_view> g_IRInstruction2Str;
extern const std::set<std::string_view> g_IRInstructionStrSet;

/**
 * Enum that contains an ID of the keywords (that are not instructions nor types).
 */
enum class IRKeywordType : uint8_t
{
    Invalid = 0,
    Module,
    Variable
};
extern const std::map<std::string_view, IRKeywordType> g_IRKeywordTypeStr2Type;
extern const std::map<IRKeywordType, std::string_view> g_IRKeywordType2Str;
extern const std::set<std::string_view> g_IRKeywordTypeStrSet;

/**
 * Enum that contains the types for different memory references.
 */
enum class IRMemoryReferenceType : uint8_t
{
    Invalid = 0,      // Not a valid memory operand
    Direct,           // Absolute address (e.g. [0x123456])
    Base,             // [base]            -> register only
    BaseDisplacement, // [base + disp]     -> reg + displacement. This includes (x86 real mode / legacy) with segment
                      // addressing.
    IndexScale,       // [index * scale] -> reg * scale + displacement
    BaseIndexScaleDisplacement, // [base + index * scale + disp] -> baseReg + indexReg * scale + displacement
    IPRelative,                 // [IP(instruction pointer) + disp] (for position-independent code)
};

enum class IRType : uint8_t
{
    Invalid = 0,
    _float,  // 32 bits.
    _double, // 64 bits.
    i8,      // 8 bits.
    i16,     // 16 bits.
    i32,     // 32 bits.
    i64,     // 64 bits.
    ptr,     // Pointer aka memory.
    string   // A string.
};
/**
 * Returns the maximum value of an integer given its IRType.
 * @param type
 * @return uint64_t
 */
extern uint64_t getMaxIRIntValue(IRType type);
extern const std::map<IRType, std::string_view> g_IRTypes2Str;
extern const std::map<std::string_view, IRType> g_IRStr2Types;
extern const std::set<std::string_view> g_IRTypesStrSet;

#endif // EZPACKER_IRTYPES_H
