#ifndef EZPACKER_IDECODEDINSTRUCTIONPARSER_H
#define EZPACKER_IDECODEDINSTRUCTIONPARSER_H

#include "EzLifterCommon.h"
#include "IDecodedOperand.h"
#include "IDecoderDefines.h"

/**
 * Enum that contains the types for different memory references.
 */
enum class MemoryReferenceType : uint8_t
{
    Invalid = 0,      // Not a valid memory operand
    Direct,           // Absolute address (e.g. [0x123456])
    Base,             // [base]            -> register only
    BaseDisplacement, // [base + disp]     -> reg + displacement. This includes (x86 real mode / legacy) with segment
                      // addressing.
    IndexScaleDisplacement,     // [index * scale + disp] -> reg * scale + displacement
    BaseIndexScaleDisplacement, // [base + index * scale + disp] -> baseReg + indexReg * scale + displacement
    IPRelative,                 // [IP(instruction pointer) + disp] (for position-independent code)
};

/**
 * Enum that contains the types for different flags.
 */
enum class FlagType : uint8_t
{
    None = 0,
    Auxiliary = (1 << 1), // Auxiliary flag (AF)
    Carry = (1 << 2),     // Carry flag (CF)
    Negative = (1 << 3),  // Negative flag (NF)
    Overflow = (1 << 4),  // Overflow flag (OF)
    Parity = (1 << 5),    // Parity flag (PF)
    Sign = (1 << 6),      // Sign flag (SF)
    Zero = (1 << 7),      // Zero flag (ZF)
};

/**
 * Enum used to know what condition is checked on branch instructions or conditional moves (yet to support). Not ordered
 * alphabetically.
 */
enum class ConditionType : uint8_t
{
    Invalid = 0,
    None,         // No condition is used
    Equal,        // ZF = 1
    NotEqual,     // ZF = 0
    Less,         // SF != OF
    LessEqual,    // ZF == 1 || SF != OF
    Greater,      // ZF == 0 && SF == OF
    GreaterEqual, // SF == OF
    Parity,       // PF = 1
    NotParity,    // PF = 0
    Negative,     // NF = 1
    NotNegative,  // NF = 0
    Overflow,     // OF = 1
    NotOverflow,  // OF = 0
    Signed,       // SF = 1
    NotSigned     // SF = 0
};

/**
 * Enum that contains the very basic instructions that are needed to be able from most of the architectures.
 */
enum class DecodedInstructionType : uint8_t
{
    Invalid = 0,     // Invalid or unrecognized instruction
    Unsupported = 1, // Used to mark certain instructions as unsupported.
    _Mov = 2,        /* For architectures that use MOVs, operands are needed to know if the instruction
                      * is a loadMem / storeMem, a movRegister or a movRegisterImm. That's why we need a new type to indicate
                      * that    "further" research is needed. If an instruction is of this type, use isLoad / isStore /
                      * isMovRegister    to know its real type.
                      */

    Add,                                     // Addition
    And,                                     // Bitwise AND
    Branch,                                  // Conditional branch (e.g., JE, BNE)
    Call,                                    // Function call
    Compare,                                 // Comparison instruction
    Divide,                                  // Division
    Exchange,                                // Exchange 2 operands.
    Jump,                                    // Unconditional jump
    LoadEffectiveAddress,                    // Compute and load effective address (e.g., LEA)
    Multiply,                                // Multiplication
    Nop,                                     // No operation
    Not,                                     // Bitwise NOT
    Or,                                      // Bitwise OR
    Pop,                                     // Pop from stack
    UNSUPPORTED_INSTRUCTION_TYPE(Prefetch),  // Memory prefetch. Adds given block to CACHE (L1, L2, L3).
    Push,                                    // Push to stack
    Return,                                  // Function return
    RotateLeft,                              // Bitwise rotate left
    RotateRight,                             // Bitwise rotate right
    SetFlags,                                // Set or clear flags
    ShiftLeft,                               // Bitwise shift left
    ShiftRight,                              // Bitwise shift right
    SignExtend,                              // Sign extension
    Subtract,                                // Subtraction
    UNSUPPORTED_INSTRUCTION_TYPE(SysCall),   // System call or interrupt
    Test,                                    // Bitwise test
    Xor,                                     // Bitwise XOR
    UNSUPPORTED_INSTRUCTION_TYPE(ZeroExtend) // Zero extension, specific of ARM.
};

/**
 * Class used to parse vital information from decoded instructions. Used not to make the lifter dependant on the
 * decoder and to have this information stored in variables so it doesn't need to be generated every time it's needed.
 * Important notes about parsed decoded instructions:
 *  - They can't have 2 memory operands.
 *  - Memory operands are one of the types in MemoryReferenceType.
 *  - Branches work with register+displacement and 32-bit encoded displacements from the current IP (short jumps).
 * Instruction type can be checked with getType but there's also a few other methods included for more readability in
 * the code base.
 */
class IDecodedInstructionParser
{
  public:
    virtual ~IDecodedInstructionParser() = default;
    
    /**
     * Returns true if the instruction is a branch (conditional / short JUMPs).
     * @return bool
     */
    virtual bool isBranch() const = 0;

    /**
     * Returns true if this instruction is a call.
     * @return bool
     */
    virtual bool isCall() const = 0;
    
    /**
     * Returns true if the instruction is a JUMP instruction. Remember jump and branch instructions are different.
     * @return bool
     */
    virtual bool isJump() const = 0;

    /**
     * Returns true if the instruction is a LOAD (moves data from memory to a register).
     * @param operands
     * @return bool
     */
    virtual bool isLoad(const std::vector<std::shared_ptr<IDecodedOperand>> &operands) const = 0;

    /**
     * Returns if this instruction moves a register to another.
     * @param operands
     * @return bool.
     */
    virtual bool isMovRegister(const std::vector<std::shared_ptr<IDecodedOperand>> &operands) const = 0;

    /**
     * Returns if this instruction moves an immediate to a register.
     * @param operands
     * @return bool.
     */
    virtual bool isMovRegisterImm(const std::vector<std::shared_ptr<IDecodedOperand>> &operands) const = 0;

    /**
     * Returns true if this instruction is a return.
     * @return bool
     */
    virtual bool isRet() const = 0;

    /**
     * Returns true if this instruction is a STORE (moves data from register to memory).
     * @param operands
     * @return bool
     */
    virtual bool isStore(const std::vector<std::shared_ptr<IDecodedOperand>> &operands) const = 0;

    /**
     * Does the instruction use memory?
     * @param operands
     * @return bool
     */
    virtual bool usesMemory(const std::vector<std::shared_ptr<IDecodedOperand>> &operands) const = 0;

    /**
     * Returns the condition type of the instruction: How does it interact with read flags.
     * @return ConditionType
     */
    virtual ConditionType getConditionType() const = 0;

    /**
     * Returns the type of the instruction.
     * @return DecodedInstructionType
     */
    virtual DecodedInstructionType getType() const = 0;

    /**
     * Returns the memory reference type for this instruction. If !usesMemory, Invalid is returned.
     * @param operands
     * @return const MemoryReferenceType &
     */
    virtual MemoryReferenceType getMemoryRefType(const std::vector<std::shared_ptr<IDecodedOperand>> &operands) const = 0;

    /**
     * Returns flags that are READ.
     * @return uint8_t.
     */
    virtual uint8_t getReadFlags() const = 0;

    /**
     * Returns flags that are WRITTEN by this instruction.
     * @return uint8_t
     */
    virtual uint8_t getWrittenFlags() const = 0;
};

#endif // EZPACKER_IDECODEDINSTRUCTIONPARSER_H
