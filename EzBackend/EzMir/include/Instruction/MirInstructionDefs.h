/**
 * @file MirInstructionDefs.h
 * @brief Compile-time instruction catalogue: opcodes, flags, categories, and metadata.
 */
#ifndef EZPACKER_MIRINSTRUCTIONDEFS_H
#define EZPACKER_MIRINSTRUCTIONDEFS_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <initializer_list>

enum class ExpectedOperandType : uint16_t
{
    None = 0,
    Register = 1 << 0,        // MirRegister
    Integer = 1 << 1,         // MirInteger
    Double = 1 << 2,          // MirDouble
    ConstantPoolRef = 1 << 3, // MirConstantPoolRef
    Memory = 1 << 4,          // MirMemory
    FrameIndex = 1 << 5,      // MirFrameIndex
    Reference = 1 << 6,       // MirReference (Blocks, Functions)

    // --- Composite Helper Masks ---

    // Standard Immediate (Raw numbers)
    Immediate = Integer | Double,

    // Register or Immediate (Standard ALU inputs)
    RegImm = Register | Integer | Double,

    // Any kind of memory address source (used for LEA)
    AddressSource = Memory | FrameIndex | ConstantPoolRef,

    // Anything that can be read as a value
    AnyValue = Register | Integer | Double | ConstantPoolRef,

    Any = 0xFFFF
};

inline std::map<ExpectedOperandType, std::string> g_ExpectedOperandType2Str = {
    { ExpectedOperandType::None, "None" },           { ExpectedOperandType::Register, "Register" },
    { ExpectedOperandType::Immediate, "Immediate" }, { ExpectedOperandType::Reference, "Reference" },
    { ExpectedOperandType::RegImm, "RegImm" },       { ExpectedOperandType::Any, "Any" }
};

inline constexpr ExpectedOperandType operator|(ExpectedOperandType a, ExpectedOperandType b)
{
    return static_cast<ExpectedOperandType>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}
inline constexpr bool operator&(ExpectedOperandType a, ExpectedOperandType b)
{
    return (static_cast<uint16_t>(a) & static_cast<uint16_t>(b)) != 0;
}

enum class OperandFlag : uint8_t
{
    None = 0,
    Read = 1 << 0,
    Write = 1 << 1,
    ReadWrite = Read | Write
};

inline constexpr OperandFlag operator|(OperandFlag a, OperandFlag b)
{
    return static_cast<OperandFlag>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline constexpr bool operator&(OperandFlag a, OperandFlag b)
{
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

struct OperandConstraint
{
    ExpectedOperandType type;
    OperandFlag flags;
};

// --- Instruction Flags (Unchanged) ---
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
    TreatAsSigned = 1 << 13
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

enum class MirInstructionCategory : uint8_t
{
    Invalid = 0,
    Array,
    DataMovement,
    Memory,
    Arithmetic,
    Bitwise,
    Compare,
    ControlFlow,
    Casting,
    System
};

inline std::map<MirInstructionCategory, std::string> g_MirInstructionCategory2Str = {
    { MirInstructionCategory::Invalid, "Invalid" },           { MirInstructionCategory::Array, "Array" },
    { MirInstructionCategory::DataMovement, "DataMovement" }, { MirInstructionCategory::Memory, "Memory" },
    { MirInstructionCategory::Arithmetic, "Arithmetic" },     { MirInstructionCategory::Bitwise, "Bitwise" },
    { MirInstructionCategory::Compare, "Compare" },           { MirInstructionCategory::ControlFlow, "ControlFlow" },
    { MirInstructionCategory::Casting, "Casting" },           { MirInstructionCategory::System, "System" }
};

// --- OpCode Generation ---
enum MirInstructionOpCode : uint16_t
{
#define INSTRUCTION(name, category, linearEquivalent, operands, flags) name,
#include "MirInstructionSet.h"
#undef INSTRUCTION
    OPCODE_COUNT
};

/**
 * This structure contains information about how an instruction can be substituted into 2 smaller instructions.
 */
struct MirInstructionLinearEquivalent
{
    MirInstructionOpCode m_high;
    MirInstructionOpCode m_low;
};

// --- Metadata Structure ---
struct MirInstructionMetadata
{
    MirInstructionCategory m_category;
    MirInstructionLinearEquivalent m_linearEquivalent;
    MirInstructionOpCode m_opcode;
    MirInstructionFlags m_flags;
    std::string_view m_name;
    std::vector<OperandConstraint> m_operandConstraints;

    MirInstructionMetadata(MirInstructionCategory category,
                           MirInstructionLinearEquivalent linearEquivalent,
                           MirInstructionOpCode opcode,
                           MirInstructionFlags flag,
                           std::string_view name,
                           std::initializer_list<OperandConstraint> operands) :
        m_category(category), m_linearEquivalent(linearEquivalent), m_opcode(opcode), m_flags(flag),
        m_name(std::move(name)), m_operandConstraints(operands)
    {
    }
};

extern std::string StrToLower(const std::string &str);

// --- Metadata Arrays ---
inline const MirInstructionMetadata g_MirInstructionSet[] = {
#define INSTRUCTION(name, category, linearEquivalent, operands, flags)                                                 \
    MirInstructionMetadata(MirInstructionCategory::category, linearEquivalent, name, flags, #name, operands),
#include "MirInstructionSet.h"
#undef INSTRUCTION
};

inline std::map<std::string, MirInstructionOpCode> g_String2MirInstruction = {
#define INSTRUCTION(name, category, linearEquivalent, operands, flags) { #name, name },
#include "MirInstructionSet.h"
#undef INSTRUCTION
};

inline const MirInstructionMetadata &getMeta(MirInstructionOpCode op) { return g_MirInstructionSet[op]; }

inline MirInstructionOpCode getOpCodeFromStr(const std::string &str)
{
    auto it = g_String2MirInstruction.find(StrToLower(str));
    if (it == g_String2MirInstruction.end())
    {
        return static_cast<MirInstructionOpCode>(0);
    }
    return it->second;
}

#endif // EZPACKER_MIRINSTRUCTIONDEFS_H