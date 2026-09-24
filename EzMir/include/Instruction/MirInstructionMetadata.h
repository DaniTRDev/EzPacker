#ifndef EZMIR_MIR_INSTRUCTION_METADATA_H
#define EZMIR_MIR_INSTRUCTION_METADATA_H

#include "EzMirCommon.h"
#include <array>
#include <string_view>

/**
 * Bitmask enumeration defining expected operand types for MIR instructions and verification.
 */
enum class ExpectedOperandType : uint16_t
{
    None = 1 << 0,           // No operand expected
    Register = 1 << 1,       // Virtual or physical register operand (MirRegister)
    Integer = 1 << 2,        // Immediate integer constant (MirInteger)
    FloatingPoint = 1 << 3,  // Immediate floating-point constant (MirFloat)
    Memory = 1 << 4,         // Memory reference addressing operand (MirMemory)
    Reference = 1 << 5,      // Symbolic reference (MirReference to Block, Function, Global, Stack slot)
    RuntimeSymbol = 1 << 6,  // Named runtime library symbol (MirRuntimeSymbol)
    VariadicArgs = (1 << 7), // Variadic argument expansion slot

    // --- Composite Helper Masks ---

    // Standard Immediate (Raw numbers)
    Immediate = Integer | FloatingPoint,

    // Register or Immediate (Standard ALU inputs)
    RegIntImm = Register | Integer,
    RegFloatImm = Register | FloatingPoint,
    RegImm = RegIntImm | RegFloatImm,

    // Any kind of memory address source
    AddressSource = Memory | Reference,

    // Anything that can be read as a value
    AnyValue = Register | Integer | FloatingPoint,

    // Matches any operand type
    Any = 0xFFFF
};

/**
 * Bitwise OR operator combining expected operand types.
 */
inline constexpr ExpectedOperandType operator|(ExpectedOperandType a, ExpectedOperandType b)
{
    return static_cast<ExpectedOperandType>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}

/**
 * Bitwise AND operator testing expected operand type intersection.
 */
inline constexpr bool operator&(ExpectedOperandType a, ExpectedOperandType b)
{
    return (static_cast<uint16_t>(a) & static_cast<uint16_t>(b)) != 0;
}

/**
 * Dataflow direction and usage flags for instruction operands (Read, Write, ReadWrite).
 */
enum class MirOperandFlag : uint8_t
{
    None = 0,
    Read = 1 << 0,           // Operand is consumed/read by the instruction (USE)
    Write = 1 << 1,          // Operand is defined/written by the instruction (DEF)
    ReadWrite = Read | Write // Operand is modified (both DEF and USE)
};

/**
 * Bitwise OR operator combining operand access flags.
 */
inline constexpr MirOperandFlag operator|(MirOperandFlag a, MirOperandFlag b)
{
    return static_cast<MirOperandFlag>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

/**
 * Bitwise AND operator testing operand access flag intersection.
 */
inline constexpr bool operator&(MirOperandFlag a, MirOperandFlag b)
{
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

/**
 * Metadata descriptor associating an expected operand type with its dataflow access flag.
 */
struct MirOperandMetadata
{
    /**
     * Bitmask of accepted operand types for this argument position.
     */
    ExpectedOperandType type;

    /**
     * Dataflow direction (Read, Write, ReadWrite) for this argument position.
     */
    MirOperandFlag flags;
};

/**
 * Maximum number of declared operand slots a single instruction metadata record can hold. Variadic
 * operand slots expand at runtime and never add metadata entries.
 */
inline constexpr size_t MirMaxOperandSlots = 8;

/**
 * Fixed-capacity operand-slot list used by the generated instruction metadata table. It is a literal
 * type, so the whole table is constant-initialized and static initialization never allocates.
 * The variadic slot position, when present, is simply the index of the entry whose type includes
 * ExpectedOperandType::VariadicArgs.
 */
struct MirOperandMetadataList
{
    std::array<MirOperandMetadata, MirMaxOperandSlots> m_slots{};
    uint8_t m_count{ 0 };

    constexpr MirOperandMetadataList() = default;

    constexpr MirOperandMetadataList(std::initializer_list<MirOperandMetadata> slots) :
        m_count(static_cast<uint8_t>(slots.size()))
    {
        size_t i = 0;
        for (const MirOperandMetadata &slot : slots)
        {
            // at() fails constant evaluation (and throws at runtime) if a declaration ever exceeds
            // the fixed capacity instead of silently truncating its operand signature.
            m_slots.at(i++) = slot;
        }
    }
};

/**
 * Semantic and behavioral flags associated with a MIR instruction opcode.
 */
enum class MirInstructionFlags : uint32_t
{
    None = 0,
    SizeMatch = 1 << 0,       // All operands must share identical bit-width
    DestLarger = 1 << 1,      // Destination operand bit-width must exceed source bit-width
    DestSmaller = 1 << 2,     // Destination operand bit-width must be smaller than source bit-width
    ReadsMemory = 1 << 3,     // Instruction performs memory load operations
    WritesMemory = 1 << 4,    // Instruction performs memory store operations
    IsTerminator = 1 << 5,    // Instruction terminates a basic block (branches, jumps, returns)
    IsBranch = 1 << 6,        // Conditional or unconditional control-flow branch
    IsCall = 1 << 7,          // Procedure call instruction
    IsReturn = 1 << 8,        // Function return instruction
    HasSideEffect = 1 << 9,   // Instruction has unmodeled side effects preventing DCE
    IsCommutative = 1 << 10,  // Binary operation is commutative: op(a, b) == op(b, a)
    ReadsCPUFlags = 1 << 11,  // Instruction inspects hardware status flags
    WritesCPUFlags = 1 << 12, // Instruction modifies hardware status flags
    TreatAsSigned = 1 << 13,  // Arithmetic or comparison treats operands as signed integers
    VariadicArgs = 1 << 14,   // Instruction accepts variable number of operands (e.g. CALL, PHI)
    IsMove = 1 << 15          // Instruction is a register-to-register or direct value move
};

/**
 * Bitwise OR operator combining instruction behavior flags.
 */
inline constexpr MirInstructionFlags operator|(MirInstructionFlags a, MirInstructionFlags b)
{
    return static_cast<MirInstructionFlags>(static_cast<std::underlying_type_t<MirInstructionFlags>>(a) |
                                            static_cast<std::underlying_type_t<MirInstructionFlags>>(b));
}

/**
 * Bitwise AND operator testing instruction behavior flag intersection.
 */
inline constexpr bool operator&(MirInstructionFlags a, MirInstructionFlags b)
{
    return (static_cast<std::underlying_type_t<MirInstructionFlags>>(a) &
            static_cast<std::underlying_type_t<MirInstructionFlags>>(b)) != 0;
}

/**
 * Functional category classifying the high-level role of an instruction.
 */
enum MirInstructionCategory : uint8_t
{
    MirCat_Invalid = 0,  // Invalid or uninitialized category
    MirCat_DataMovement, // Register moves, constant loading (MOV)
    MirCat_Memory,       // Memory loads and stores (LOAD, STORE)
    MirCat_Arithmetic,   // Arithmetic computations (ADD, SUB, MUL, DIV, NEG)
    MirCat_Bitwise,      // Bitwise logic and shifts (AND, OR, XOR, SHL, SHR)
    MirCat_Compare,      // Relational comparisons (CMP_EQ, CMP_NE, CMP_LT, etc.)
    MirCat_ControlFlow,  // Branches, jumps, calls, returns, phi nodes (BR, JMP, CALL, RET, PHI)
    MirCat_Casting,      // Type conversions, truncations, extensions (CAST, TRUNC, ZEXT, SEXT)
    MirCat_System,       // System calls, interrupts, inline assembly
    MirCat_Vector        // Vector/SIMD computations and data manipulation
};

/**
 * Compilation tier indicating the abstraction level of an instruction.
 */
enum class MirInstructionTier : uint8_t
{
    HighLevel,    // Standard IR opcodes emitted by the frontend/IRBuilder (ADD, SUB, CALL, RET, etc.)
    PassInternal, // Intermediate lowering opcodes generated/consumed by passes (PUSH_ARG, POP_ARG, PUSH_RET, POP_RET,
                  // etc.)
    TargetLow     // Machine-specific target instructions produced by instruction selection
};

// --- Metadata Structure ---
enum class MirInstructionOpCode : uint16_t;

/**
 * Static metadata descriptor capturing complete classification and operand schema for an opcode.
 */
struct MirInstructionMetadata
{
    /**
     * Functional category of the opcode.
     */
    MirInstructionCategory m_category;

    /**
     * Concrete opcode enumeration value.
     */
    MirInstructionOpCode m_opcode;

    /**
     * Abstraction tier of the instruction.
     */
    MirInstructionTier m_tier;

    /**
     * Behavioral and semantic flags.
     */
    MirInstructionFlags m_flags;

    /**
     * Mnemonic name of the opcode.
     */
    std::string_view m_name;

    /**
     * Formal operand signature specifying expected types and access directions. Fixed-capacity
     * storage keeps the generated table constant-initialized with no per-opcode heap allocation.
     */
    MirOperandMetadataList m_operandMeta;

    /**
     * Constructs a static metadata descriptor for an instruction opcode.
     */
    constexpr MirInstructionMetadata(MirInstructionCategory category,
                                     MirInstructionOpCode opcode,
                                     MirInstructionTier tier,
                                     MirInstructionFlags flag,
                                     std::string_view name,
                                     MirOperandMetadataList operands) :
        m_category(category), m_opcode(opcode), m_tier(tier), m_flags(flag), m_name(name),
        m_operandMeta(operands)
    {
    }
};
#endif // EZMIR_MIR_INSTRUCTION_METADATA_H