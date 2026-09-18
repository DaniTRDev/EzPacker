#ifndef EZMIR_MIR_INSTRUCTION_SET_H
#define EZMIR_MIR_INSTRUCTION_SET_H

#include "MirInstructionMetadata.h"

// --- OpCode Generation ---
/**
 * Enumeration of all MIR instruction opcodes generated from MirInstructionSetDefs.h.
 */
enum class MirInstructionOpCode : uint16_t
{
#define INSTRUCTION(name, tier, category, operands, flags) name,
#include "MirInstructionSetDefs.h"
#undef INSTRUCTION
    OPCODE_COUNT
};

/**
 * Global array storing static opcode metadata records indexed by MirInstructionOpCode value.
 */
inline const MirInstructionMetadata g_MirInstructionSet[] = {
#define INSTRUCTION(name, tier, category, operands, flags)                                                             \
    MirInstructionMetadata(MirInstructionCategory::category, MirInstructionOpCode::name, tier, flags, #name, operands),
#include "MirInstructionSetDefs.h"
#undef INSTRUCTION
};

/**
 * Mapping table from opcode name strings to their MirInstructionOpCode enum values.
 */
inline std::unordered_map<std::string, MirInstructionOpCode> g_String2MirInstruction = {
#define INSTRUCTION(name, tier, category, operands, flags) { #name, MirInstructionOpCode::name },
#include "MirInstructionSetDefs.h"
#undef INSTRUCTION
};

/**
 * Retrieves the static metadata descriptor for the given MIR opcode.
 */
inline const MirInstructionMetadata &getMeta(MirInstructionOpCode op)
{
    return g_MirInstructionSet[static_cast<uint16_t>(op)];
}

/**
 * Parses a string representation of an opcode into its MirInstructionOpCode enum value (case-insensitive).
 * Returns opcode 0 if no match is found.
 */
inline MirInstructionOpCode getOpCodeFromStr(const std::string &str)
{
    auto it = g_String2MirInstruction.find(str);
    if (it != g_String2MirInstruction.end())
    {
        return it->second;
    }
    std::string upper = StrToUpper(str);
    it = g_String2MirInstruction.find(upper);
    if (it != g_String2MirInstruction.end())
    {
        return it->second;
    }
    it = g_String2MirInstruction.find(StrToLower(str));
    if (it != g_String2MirInstruction.end())
    {
        return it->second;
    }
    // Alias 'BR' to 'JMP' (standard unconditional branch)
    if (upper == "BR")
    {
        return MirInstructionOpCode::JMP;
    }
    // Alias 'ICMP_*' to 'CMP_*' (LLVM-style integer comparisons)
    if (upper.rfind("ICMP_", 0) == 0)
    {
        std::string cmpStr = "CMP_" + upper.substr(5);
        it = g_String2MirInstruction.find(cmpStr);
        if (it != g_String2MirInstruction.end())
        {
            return it->second;
        }
    }
    return static_cast<MirInstructionOpCode>(0);
}

#endif // EZMIR_MIR_INSTRUCTION_SET_H