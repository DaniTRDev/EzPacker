#ifndef EZDSL_CPP_TARGET_INST_GENERATOR_H
#define EZDSL_CPP_TARGET_INST_GENERATOR_H

#include "EzDslCommon.h"
#include <filesystem>
#include <string_view>

class DiagnosticCollector;
class SymbolTable;

namespace CodeGenerators
{

/**
 * Controls which file artifacts are synthesized during the target instruction generation pass.
 */
enum class TargetInstGenWorkingMode : uint8_t
{
    Header = 1,             // Generates ONLY the instruction definition header (<Target>InstructionDefs.h).
    Source = 1 << 1,        // Generates ONLY the definition source file (<Target>InstructionDefs.cpp).
    Encoder = 1 << 2,       // Generates ONLY the binary encoder translation unit (<Target>BinaryEncoder.cpp).
    Full = Header | Source | Encoder // Generates all instruction definition and encoding artifacts.
};

constexpr TargetInstGenWorkingMode operator|(TargetInstGenWorkingMode a, TargetInstGenWorkingMode b) noexcept
{
    return static_cast<TargetInstGenWorkingMode>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
constexpr bool operator&(TargetInstGenWorkingMode a, TargetInstGenWorkingMode b) noexcept
{
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0;
}

/**
 * Synthesizes target instruction definitions, opcodes, operand descriptors, binary encoders,
 * and assembly template disassembler logic from EzDSL .idf metadata.
 *
 * ### Synthesized Artifacts:
 * 1. `<Target>InstructionDefs.h`:
 *    - `enum class TargetOpCode : uint16_t` with opcode numbering starting at `TARGET_OPCODE_START = 1000`.
 *    - `struct MirTargetInstructionDesc` declaring operand constraints, implicit defs/uses, latency, and flags.
 *    - Lookup helpers (`GetTargetInstructionDesc`, `GetTargetOpCodeName`, `GetTargetOpCodeByName`).
 *
 * 2. `<Target>InstructionDefs.cpp`:
 *    - Static descriptor array holding target instruction metadata.
 *    - Assembly template disassembler printer (`PrintTargetInstruction`).
 *
 * 3. `<Target>BinaryEncoder.cpp`:
 *    - Bitfield packing encoders (`Encode_<OpCode>`) emitting encoded bytes directly into `CodeSection`.
 *    - Target instruction dispatch encoder (`EncodeTargetInstruction`).
 */
extern bool GenerateTargetInstructionDefs(class DiagnosticCollector *collector,
                                          class SymbolTable *table,
                                          std::filesystem::path outPath,
                                          std::string_view targetName = "",
                                          TargetInstGenWorkingMode mode = TargetInstGenWorkingMode::Full);

} // namespace CodeGenerators

#endif // EZDSL_CPP_TARGET_INST_GENERATOR_H
