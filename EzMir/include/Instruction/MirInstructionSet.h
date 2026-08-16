#ifndef EZMIR_MIR_INSTRUCTION_SET_H
#define EZMIR_MIR_INSTRUCTION_SET_H

#include "MirInstructionMetadata.h"

// --- OpCode Generation ---
enum MirInstructionOpCode : uint16_t
{
#define INSTRUCTION(name, tier, category, operands, flags) name,
#include "MirInstructionSetDefs.h"
#undef INSTRUCTION
    OPCODE_COUNT
};

// --- Metadata Arrays ---
inline const MirInstructionMetadata g_MirInstructionSet[] = {
#define INSTRUCTION(name, tier, category, operands, flags)                                                             \
    MirInstructionMetadata(MirInstructionCategory::category, name, tier, flags, #name, operands),
#include "MirInstructionSetDefs.h"
#undef INSTRUCTION
};

inline std::unordered_map<std::string, MirInstructionOpCode> g_String2MirInstruction = {
#define INSTRUCTION(name, tier, category, operands, flags) { #name, name },
#include "MirInstructionSetDefs.h"
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

#endif // EZMIR_MIR_INSTRUCTION_SET_H