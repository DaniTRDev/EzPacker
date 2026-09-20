#ifndef EZCODEEMITTER_X86_64_INSTRUCTION_ENCODER_H
#define EZCODEEMITTER_X86_64_INSTRUCTION_ENCODER_H

#include "X86_64/Encoding/X86_64EncodingDesc.h"
#include "Encoding/EncodeResult.h"

#include <cstdint>
#include <span>
#include <vector>

namespace EzCodeEmitter::X86_64
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

    /**
     * Emits a register-to-register move using the target's standard move encoding.
     *
     * Used by emitters to materialize the copy implied by a two-address instruction
     * whose destination and first source differ, without resorting to a bespoke
     * per-target move helper.
     */
    static void encodeRegisterMove(const ResolvedOperand &dst, const ResolvedOperand &src, std::vector<uint8_t> &out);

    /**
     * Emits a short unconditional branch (2-byte displacement field).
     */
    static void emitJmpShort(std::vector<uint8_t> &out, int8_t disp);

    /**
     * Emits a near unconditional branch (4-byte displacement field).
     */
    static void emitJmpNear(std::vector<uint8_t> &out, int32_t disp);

    /**
     * Emits a short conditional branch (2-byte displacement field).
     */
    static void emitJccShort(std::vector<uint8_t> &out, ConditionCode cc, int8_t disp);

    /**
     * Emits a near conditional branch (4-byte displacement field).
     */
    static void emitJccNear(std::vector<uint8_t> &out, ConditionCode cc, int32_t disp);

    /// Opcode of a near unconditional jump (JMP rel32).
    static constexpr uint8_t kNearJmpOpcode = 0xE9;
    /// Opcode of a near call (CALL rel32).
    static constexpr uint8_t kNearCallOpcode = 0xE8;
    /// Two-byte opcode prefix of a near conditional jump (Jcc rel32).
    static constexpr uint8_t kNearJccPrefix = 0x0F;
    /// Base opcode of a near conditional jump; the condition code occupies the low nibble.
    static constexpr uint8_t kNearJccBase = 0x80;

    /**
     * Classifies the near branch/call opcode at `bytes[offset]`, reporting the displacement-field
     * offset and the length of the whole instruction (both relative to `offset`). This is the
     * single source of truth for the bytes-level branch layout used by the bit patcher, so it can
     * never drift from emitJmpNear()/emitJccNear().
     *
     * Returns false for unrecognized opcodes or truncated input.
     */
    static bool classifyNearBranch(std::span<const uint8_t> bytes, size_t offset, size_t &dispOffset, size_t &instrLength);

    /**
     * Writes a 32-bit little-endian displacement at `bytes[offset]`. Returns false when the field
     * would run past the end of the buffer.
     */
    static bool writeDisp32(std::span<uint8_t> bytes, size_t offset, int32_t value);
};

} // namespace EzCodeEmitter::X86_64

#endif // EZCODEEMITTER_X86_64_INSTRUCTION_ENCODER_H
