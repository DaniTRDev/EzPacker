#ifndef EZPACKER_MIRINSTRUCTIONDEFS_H
#define EZPACKER_MIRINSTRUCTIONDEFS_H

#include "EzMirCommon.h"

enum class ExpectedOperandType : uint16_t
{
    None = 0,
    Register = 1 << 0,      // MirRegister
    Integer = 1 << 1,       // MirInteger
    FloatingPoint = 1 << 2, // MirDouble
    Memory = 1 << 4,        // MirMemory
    FrameIndex = 1 << 5,    // MirFrameIndex
    Reference = 1 << 6,     // MirReference (Blocks, Functions)
    RuntimeSymbol = 1 << 7, // MirRuntimeSymbol. Used to identify an address that's exported by the RT library.

    // --- Composite Helper Masks ---

    // Standard Immediate (Raw numbers)
    Immediate = Integer | FloatingPoint,

    // Register or Immediate (Standard ALU inputs)
    RegIntImm = Register | Integer,
    RegFloatImm = Register | FloatingPoint,
    RegImm = RegIntImm | RegFloatImm,

    // Any kind of memory address source (used for LEA)
    AddressSource = Memory | FrameIndex,

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

enum class MirInstructionTier : uint8_t
{
    HighLevel,    // Standard IR opcodes emitted by the frontend/IRBuilder (ADD, SUB, CALL, RET, etc.)
    PassInternal, // Intermediate lowering opcodes generated/consumed by passes (PUSH_ARG, POP_ARG, PUSH_RET, POP_RET,
                  // etc.)
    TargetLow     // Target-selected hardware pseudo-opcodes emitted during ISel (TARGET_INST, etc.)
};

// --- OpCode Generation ---
enum MirInstructionOpCode : uint16_t
{
#define INSTRUCTION(name, tier, category, operands, flags) name,
#include "MirInstructionSet.h"
#undef INSTRUCTION
    OPCODE_COUNT
};

// --- Metadata Structure ---
struct MirInstructionMetadata
{
    MirInstructionCategory m_category;
    MirInstructionOpCode m_opcode;
    MirInstructionTier m_tier;
    MirInstructionFlags m_flags;
    std::string_view m_name;
    std::vector<OperandConstraint> m_operandConstraints;

    MirInstructionMetadata(MirInstructionCategory category,
                           MirInstructionOpCode opcode,
                           MirInstructionTier tier,
                           MirInstructionFlags flag,
                           std::string_view name,
                           std::initializer_list<OperandConstraint> operands) :
        m_category(category), m_opcode(opcode), m_tier(tier), m_flags(flag), m_name(std::move(name)),
        m_operandConstraints(operands)
    {
    }
};

extern std::string StrToLower(const std::string &str);

// --- Metadata Arrays ---
inline const MirInstructionMetadata g_MirInstructionSet[] = {
#define INSTRUCTION(name, tier, category, operands, flags)                                                             \
    MirInstructionMetadata(MirInstructionCategory::category, name, tier, flags, #name, operands),
#include "MirInstructionSet.h"
#undef INSTRUCTION
};

inline std::map<std::string, MirInstructionOpCode> g_String2MirInstruction = {
#define INSTRUCTION(name, tier, category, operands, flags) { #name, name },
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