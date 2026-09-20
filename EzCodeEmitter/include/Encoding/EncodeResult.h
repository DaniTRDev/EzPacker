#ifndef EZCODEEMITTER_ENCODING_ENCODE_RESULT_H
#define EZCODEEMITTER_ENCODING_ENCODE_RESULT_H

#include <cstddef>
#include <cstdint>

namespace EzCodeEmitter
{

/**
 * Architecture-neutral result metadata produced alongside emitted instruction bytes.
 *
 * Every target encoder reports relocation information through this shared type so
 * the emitter layer can register fixups without knowing the concrete encoding IR
 * of the target it drives.
 */
struct EncodeResult
{
    bool m_hasReloc{ false }; ///< True when the emitted instruction contains a relocation field.
    /// Byte offset (within the emitted instruction) of the relocation field.
    size_t m_relocOffset{ 0 };
    /// Width of the relocation field in bits (8 or 32).
    uint8_t m_relocBits{ 32 };
    /// True when the relocation is a PC-relative branch (vs. a data/PC-relative fixup).
    bool m_isBranch{ false };
};

} // namespace EzCodeEmitter

#endif // EZCODEEMITTER_ENCODING_ENCODE_RESULT_H
