#ifndef EZMIR_MIR_INSTRUCTION_METADATA_H
#define EZMIR_MIR_INSTRUCTION_METADATA_H

#include "EzMirCommon.h"

/**
 * Bitmask enumeration defining expected operand types for MIR instructions and verification.
 */
enum class ExpectedOperandType : uint16_t
{
    None = 1 << 0,
    Register = 1 << 1,      // MirRegister
    Integer = 1 << 2,       // MirInteger
    FloatingPoint = 1 << 3, // MirDouble
    Memory = 1 << 4,        // MirMemory
    Reference = 1 << 5,     // MirReference (Blocks, Functions)
    // MirRuntimeSymbol. Used to identify an address, by its name, that's exported by the RT library.
    RuntimeSymbol = 1 << 6,
    // Used to define the variadic args, this is useful to define if they are read or written.
    VariadicArgs = (1 << 7),

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

    Any = 0xFFFF
};

inline constexpr ExpectedOperandType operator|(ExpectedOperandType a, ExpectedOperandType b)
{
    return static_cast<ExpectedOperandType>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}
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
    Read = 1 << 0,
    Write = 1 << 1,
    ReadWrite = Read | Write
};

inline constexpr MirOperandFlag operator|(MirOperandFlag a, MirOperandFlag b)
{
    return static_cast<MirOperandFlag>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline constexpr bool operator&(MirOperandFlag a, MirOperandFlag b)
{
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

/**
 * Metadata descriptor associating an expected operand type with its dataflow access flag.
 */
struct MirOperandMetadata
{
    ExpectedOperandType type;
    MirOperandFlag flags;
};

/**
 * Semantic and behavioral flags associated with a MIR instruction opcode.
 */
enum class MirInstructionFlags : uint32_t
{
    None = 0,
    SizeMatch = 1 << 0,
    DestLarger = 1 << 1,
    DestSmaller = 1 << 2,
    ReadsMemory = 1 << 3,
    WritesMemory = 1 << 4,
    IsTerminator = 1 << 5,
    IsBranch = 1 << 6,
    IsCall = 1 << 7,
    IsReturn = 1 << 8,
    HasSideEffect = 1 << 9,
    IsCommutative = 1 << 10,
    ReadsCPUFlags = 1 << 11,
    WritesCPUFlags = 1 << 12,
    TreatAsSigned = 1 << 13,
    VariadicArgs = 1 << 14 // Flag used to tell that there will be an unexpected number of arguments. If a constraint is
                           // given, it must be followed. By default, all the values are READ.
};

inline constexpr MirInstructionFlags operator|(MirInstructionFlags a, MirInstructionFlags b)
{
    return static_cast<MirInstructionFlags>(static_cast<std::underlying_type_t<MirInstructionFlags>>(a) |
                                            static_cast<std::underlying_type_t<MirInstructionFlags>>(b));
}

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
    MirCat_Invalid = 0,
    MirCat_DataMovement,
    MirCat_Memory,
    MirCat_Arithmetic,
    MirCat_Bitwise,
    MirCat_Compare,
    MirCat_ControlFlow,
    MirCat_Casting,
    MirCat_System
};

inline std::map<MirInstructionCategory, std::string> g_MirInstructionCategory2Str = {
    { MirCat_Invalid, "MirCat_Invalid" },
    { MirCat_DataMovement, "MirCat_DataMovement" },
    { MirCat_Memory, "MirCat_Memory" },
    { MirCat_Arithmetic, "MirCat_Arithmetic" },
    { MirCat_Bitwise, "MirCat_Bitwise" },
    { MirCat_Compare, "MirCat_Compare" },
    { MirCat_ControlFlow, "MirCat_ControlFlow" },
    { MirCat_Casting, "MirCat_Casting" },
    { MirCat_System, "MirCat_System" }
};

/**
 * Compilation tier indicating the abstraction level of an instruction.
 */
enum class MirInstructionTier : uint8_t
{
    HighLevel,    // Standard IR opcodes emitted by the frontend/IRBuilder (ADD, SUB, CALL, RET, etc.)
    PassInternal, // Intermediate lowering opcodes generated/consumed by passes (PUSH_ARG, POP_ARG, PUSH_RET, POP_RET,
                  // etc.)
    TargetLow     // For selected (by ISel) instructions.
};

// --- Metadata Structure ---
enum class MirInstructionOpCode : uint16_t;

/**
 * Static metadata descriptor capturing complete classification and operand schema for an opcode.
 */
struct MirInstructionMetadata
{
    MirInstructionCategory m_category;
    MirInstructionOpCode m_opcode;
    MirInstructionTier m_tier;
    MirInstructionFlags m_flags;
    std::string_view m_name;
    std::vector<MirOperandMetadata> m_operandFlags;

    MirInstructionMetadata(MirInstructionCategory category,
                           MirInstructionOpCode opcode,
                           MirInstructionTier tier,
                           MirInstructionFlags flag,
                           std::string_view name,
                           std::initializer_list<MirOperandMetadata> operands) :
        m_category(category), m_opcode(opcode), m_tier(tier), m_flags(flag), m_name(std::move(name)),
        m_operandFlags(operands)
    {
    }
};
#endif // EZMIR_MIR_INSTRUCTION_METADATA_H