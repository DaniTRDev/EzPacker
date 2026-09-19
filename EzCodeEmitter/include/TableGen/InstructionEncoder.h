#ifndef EZCODEEMITTER_TABLEGEN_INSTRUCTION_ENCODER_H
#define EZCODEEMITTER_TABLEGEN_INSTRUCTION_ENCODER_H

#include "TableGen/EncodingDesc.h"

#include <cstdint>
#include <span>
#include <vector>

namespace EzCodeEmitter::TableGen
{

/**
 * Target-agnostic, table-driven instruction encoder.
 *
 * A single runtime interprets an EncodingDesc together with a list of resolved operands
 * and produces machine-code bytes. Per-target differences live entirely in the generated
 * `{Target}EncodingTable`, not in this class.
 *
 * The encoder never touches relocations: PC-relative fields are emitted as zero
 * placeholders and their offset is reported through EncodeResult so the emitter layer
 * can register the appropriate fixup.
 */
class InstructionEncoder
{
  public:
    /**
     * Encodes one instruction into `out` (appending to any existing contents) and
     * reports relocation metadata via `result`.
     *
     * Returns false when the descriptor is malformed or an operand is missing/invalid;
     * in that case `out` is left untouched.
     */
    static bool encode(const EncodingDesc &desc,
                       std::span<const ResolvedOperand> operands,
                       std::vector<uint8_t> &out,
                       EncodeResult &result);

    /**
     * Encodes the ModR/M byte: (mod << 6) | ((reg & 7) << 3) | (rm & 7).
     */
    static constexpr uint8_t encodeModRM(uint8_t mod, uint8_t reg, uint8_t rm)
    {
        return static_cast<uint8_t>(((mod & 0x03u) << 6) | ((reg & 0x07u) << 3) | (rm & 0x07u));
    }

    /**
     * Encodes a SIB byte: (scalePower << 6) | ((index & 7) << 3) | (base & 7).
     */
    static constexpr uint8_t encodeSIB(uint8_t scalePower, uint8_t index, uint8_t base)
    {
        return static_cast<uint8_t>(((scalePower & 0x03u) << 6) | ((index & 0x07u) << 3) | (base & 0x07u));
    }

    /**
     * Builds a REX prefix byte from its individual bits.
     */
    static constexpr uint8_t rexByte(bool w, bool r, bool x, bool b)
    {
        return static_cast<uint8_t>(0x40u | (w ? 0x08u : 0u) | (r ? 0x04u : 0u) | (x ? 0x02u : 0u) | (b ? 0x01u : 0u));
    }
};

} // namespace EzCodeEmitter::TableGen

#endif // EZCODEEMITTER_TABLEGEN_INSTRUCTION_ENCODER_H
