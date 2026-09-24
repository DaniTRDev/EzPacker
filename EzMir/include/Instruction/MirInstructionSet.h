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
 * One opcode-name binding in the static string-to-opcode table. Replacing the previous
 * std::unordered_map removes a static-init heap allocation while keeping lookup allocation-free.
 */
struct MirInstructionNameEntry
{
    std::string_view m_name;
    MirInstructionOpCode m_opcode;
};

/**
 * Static table mapping every opcode name literal to its MirInstructionOpCode enum value. Keys are
 * string_views over the generated name literals, so the table is constant-initialized.
 */
inline constexpr MirInstructionNameEntry g_MirInstructionNames[] = {
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
 * Looks up an opcode name in the static table, returning opcode 0 when no exact match exists.
 */
inline MirInstructionOpCode findMirInstructionName(std::string_view key)
{
    for (const MirInstructionNameEntry &entry : g_MirInstructionNames)
    {
        if (entry.m_name == key)
        {
            return entry.m_opcode;
        }
    }
    return static_cast<MirInstructionOpCode>(0);
}

/**
 * Parses a string representation of an opcode into its MirInstructionOpCode enum value (case-insensitive).
 * Returns opcode 0 if no match is found. Accepts a view so callers never materialize a temporary string.
 */
inline MirInstructionOpCode getOpCodeFromStr(std::string_view str)
{
    const auto findOpCode = [](std::string_view key) -> MirInstructionOpCode
    { return findMirInstructionName(key); };

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