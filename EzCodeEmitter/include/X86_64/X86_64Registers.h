#ifndef EZPACKER_X86_64_REGISTERS_H
#define EZPACKER_X86_64_REGISTERS_H

#include "EzCodeEmitterCommon.h"

namespace EzCodeEmitter::X86_64
{

/**
 * Standard hardware encoding numbers for x86-64 general purpose registers.
 * Encoded directly in ModR/M and SIB bytes (0..15).
 */
enum class Reg : uint8_t
{
    RAX = 0,
    RCX = 1,
    RDX = 2,
    RBX = 3,
    RSP = 4,
    RBP = 5,
    RSI = 6,
    RDI = 7,
    R8  = 8,
    R9  = 9,
    R10 = 10,
    R11 = 11,
    R12 = 12,
    R13 = 13,
    R14 = 14,
    R15 = 15,
    XMM0 = 16,
    XMM1 = 17,
    XMM2 = 18,
    XMM3 = 19,
    XMM4 = 20,
    XMM5 = 21,
    XMM6 = 22,
    XMM7 = 23,
    XMM8 = 24,
    XMM9 = 25,
    XMM10 = 26,
    XMM11 = 27,
    XMM12 = 28,
    XMM13 = 29,
    XMM14 = 30,
    XMM15 = 31,
    None = 0xFF
};

/**
 * Returns numeric ID (0..31) of the register.
 */
constexpr uint8_t getRegId(Reg reg)
{
    return static_cast<uint8_t>(reg);
}

/**
 * Returns true if register is an SSE vector register (XMM0..XMM15).
 */
constexpr bool isXmmReg(Reg reg)
{
    return static_cast<uint8_t>(reg) >= 16 && static_cast<uint8_t>(reg) <= 31;
}

/**
 * Returns XMM index (0..15) for SSE vector registers.
 */
constexpr uint8_t getXmmId(Reg reg)
{
    return static_cast<uint8_t>(reg) - 16;
}

/**
 * Returns low 3 bits (0..7) used for ModR/M reg/rm fields and SIB base/index fields.
 */
constexpr uint8_t getLow3Bits(Reg reg)
{
    if (isXmmReg(reg))
    {
        return (static_cast<uint8_t>(reg) - 16) & 0x07;
    }
    return static_cast<uint8_t>(reg) & 0x07;
}

/**
 * Returns extension bit (bit 3) indicating register 8..15 requiring REX bit extension.
 */
constexpr uint8_t getExtBit(Reg reg)
{
    if (isXmmReg(reg))
    {
        return ((static_cast<uint8_t>(reg) - 16) >> 3) & 0x01;
    }
    return (static_cast<uint8_t>(reg) >> 3) & 0x01;
}

/**
 * Returns true if register ID is >= 8 (R8..R15 or XMM8..XMM15).
 */
constexpr bool isExtendedReg(Reg reg)
{
    if (isXmmReg(reg))
    {
        return static_cast<uint8_t>(reg) >= 24 && static_cast<uint8_t>(reg) <= 31;
    }
    return static_cast<uint8_t>(reg) >= 8 && static_cast<uint8_t>(reg) <= 15;
}

/**
 * Returns standard assembly name for the register given its operand size (1, 2, 4, 8 bytes).
 */
std::string_view getRegName(Reg reg, uint8_t sizeInBytes);

} // namespace EzCodeEmitter::X86_64

#endif // EZPACKER_X86_64_REGISTERS_H
