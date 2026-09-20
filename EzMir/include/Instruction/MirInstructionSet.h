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
 * Mapping table from opcode name strings to their MirInstructionOpCode enum values. Keys are
 * string_views over the static opcode-name literals, so the table performs no dynamic key
 * allocations and lookups can reuse an existing view without copying.
 */
inline std::unordered_map<std::string_view, MirInstructionOpCode> g_String2MirInstruction = {
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
 * Returns opcode 0 if no match is found. Accepts a view so callers never materialize a temporary string.
 */
inline MirInstructionOpCode getOpCodeFromStr(std::string_view str)
{
    const auto findOpCode = [](std::string_view key) -> MirInstructionOpCode
    {
        auto it = g_String2MirInstruction.find(key);
        return it != g_String2MirInstruction.end() ? it->second : static_cast<MirInstructionOpCode>(0);
    };

    if (MirInstructionOpCode op = findOpCode(str); op != static_cast<MirInstructionOpCode>(0))
    {
        return op;
    }

    // Normalize the case once; the table only holds upper-case spellings.
    const std::string upper = StrToUpper(str);
    if (MirInstructionOpCode op = findOpCode(upper); op != static_cast<MirInstructionOpCode>(0))
    {
        return op;
    }
    if (MirInstructionOpCode op = findOpCode(StrToLower(str)); op != static_cast<MirInstructionOpCode>(0))
    {
        return op;
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
        if (MirInstructionOpCode op = findOpCode(cmpStr); op != static_cast<MirInstructionOpCode>(0))
        {
            return op;
        }
    }
    return static_cast<MirInstructionOpCode>(0);
}

#endif // EZMIR_MIR_INSTRUCTION_SET_H