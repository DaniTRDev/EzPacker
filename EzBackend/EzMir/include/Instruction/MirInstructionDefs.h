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

// --- Operands & Flags (Unchanged) ---
enum class ExpectedOperandType : uint16_t
{
   None = 0,
   Register = 1 << 0,
   Immediate = 1 << 1,
   Reference = 1 << 2,

   RegImm = Register | Immediate,
   Any = Register | Immediate | Reference
};

inline std::map<ExpectedOperandType, std::string> g_ExpectedOperandType2Str = {
   { ExpectedOperandType::None, "None" },           { ExpectedOperandType::Register, "Register" },
   { ExpectedOperandType::Immediate, "Immediate" }, { ExpectedOperandType::Reference, "Reference" },
   { ExpectedOperandType::RegImm, "RegImm" },       { ExpectedOperandType::Any, "Any" }
};

inline constexpr ExpectedOperandType operator|(ExpectedOperandType a, ExpectedOperandType b) {
   return static_cast<ExpectedOperandType>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}
inline constexpr bool operator&(ExpectedOperandType a, ExpectedOperandType b) {
   return (static_cast<uint16_t>(a) & static_cast<uint16_t>(b)) != 0;
}

enum class OperandFlag : uint8_t
{
   None = 0,
   Read = 1 << 0,
   Write = 1 << 1,
   ReadWrite = Read | Write
};

inline constexpr OperandFlag operator|(OperandFlag a, OperandFlag b) {
   return static_cast<OperandFlag>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline constexpr bool operator&(OperandFlag a, OperandFlag b) {
   return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

struct OperandConstraint
{
   ExpectedOperandType type;
   OperandFlag flags;
};

// --- Instruction Flags (Unchanged) ---
enum MirInstructionFlags : uint32_t
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
   WritesCPUFlags = 1 << 12
};

// --- NEW: Instruction Categories ---
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
// Notice the new 'category' parameter in the X-macro
enum MirInstructionOpCode : uint16_t
{
#define INSTRUCTION(name, category, operands, flags) name,
#include "MirInstructionSet.h"
#undef INSTRUCTION
   LOWERED,
   OPCODE_COUNT
};

// --- Metadata Structure ---
struct MirInstructionMetadata
{
   MirInstructionOpCode m_opcode;
   MirInstructionCategory m_category; // <-- Added Category
   std::vector<OperandConstraint> m_operands;
   uint32_t m_flags;
   std::string m_name;

   MirInstructionMetadata(MirInstructionOpCode op,
                          MirInstructionCategory cat,
                          std::initializer_list<OperandConstraint> ops,
                          uint32_t flg,
                          std::string nm)
       : m_opcode(op), m_category(cat), m_operands(ops), m_flags(flg), m_name(std::move(nm))
   {
   }
};

extern std::string StrToLower(const std::string &str);

// --- Metadata Arrays ---
inline const MirInstructionMetadata g_MirInstructionSet[] = {
#define INSTRUCTION(name, category, operands, flags) \
   MirInstructionMetadata(name, MirInstructionCategory::category, operands, flags, StrToLower(#name)),
#include "MirInstructionSet.h"
#undef INSTRUCTION
};

inline std::map<std::string, MirInstructionOpCode> g_String2MirInstruction = {
#define INSTRUCTION(name, category, operands, flags) { StrToLower(#name), name },
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