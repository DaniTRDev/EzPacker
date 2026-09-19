#include "Predicates/Predicates.h"
#include "Function/MirFunctionRegisterInfo.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionSet.h"
#include "Operand/MirOperand.h"
#include "Operand/MirOperands.h"
#include "Type/MirType.h"

namespace Predicates
{
/// Returns the integer constant payload of operand, or nullptr when it is not an integer immediate.
static const FlexInt *extractFlexInt(const MirOperand *operand)
{
    if (!operand || operand->getType() != MirOperandType::Integer)
        return nullptr;

    const auto *imm = static_cast<const MirInteger *>(operand);
    return &imm->getValue();
}

/// Returns the float constant payload of operand, or nullptr when it is not a float immediate.
static const FlexFloat *extractFlexFloat(const MirOperand *operand)
{
    if (!operand || operand->getType() != MirOperandType::FloatingPoint)
        return nullptr;

    const auto *flt = static_cast<const MirFloat *>(operand);
    return &flt->getValue();
}

/// Returns the operand's width in bits derived from its MIR type, or 0 when unavailable.
static uint32_t getOperandBitWidth(const MirOperand *operand)
{
    if (!operand || !operand->getMirType())
        return 0;
    return static_cast<uint32_t>(operand->getSizeInBytes() * CHAR_BIT);
}

bool hasOneUse(const MirOperand *operand, const MirFunctionRegisterInfo *regInfo)
{
    if (!operand || !regInfo || operand->getType() != MirOperandType::Register)
        return false;

    const auto *reg = static_cast<const MirRegister *>(operand);
    if (!reg->isVirtual())
        return false;

    return regInfo->hasOneUse(reg->getRegId());
}

MirInstruction *getDefiningInstr(const MirOperand *operand, const MirFunctionRegisterInfo *regInfo)
{
    if (!operand || !regInfo || operand->getType() != MirOperandType::Register)
        return nullptr;

    const auto *reg = static_cast<const MirRegister *>(operand);
    if (!reg->isVirtual())
        return nullptr;

    return regInfo->getDef(reg->getRegId());
}

bool isDefinedByOpcode(const MirOperand *operand, MirInstructionOpCode opcode, const MirFunctionRegisterInfo *regInfo)
{
    MirInstruction *def = getDefiningInstr(operand, regInfo);
    if (!def)
        return false;

    return def->getOpCode() == opcode;
}

bool canFoldWithoutSideEffects(const MirOperand *operand, const MirFunctionRegisterInfo *regInfo)
{
    MirInstruction *def = getDefiningInstr(operand, regInfo);
    if (!def)
        return false;

    // Bitmask of flags that prevent safe instruction folding and reordering
    constexpr MirInstructionFlags ForbiddenFlags = MirInstructionFlags::HasSideEffect |
            MirInstructionFlags::ReadsMemory | MirInstructionFlags::WritesMemory | MirInstructionFlags::IsTerminator |
            MirInstructionFlags::IsBranch | MirInstructionFlags::IsCall | MirInstructionFlags::IsReturn;

    // 1. Check instruction behavioral flags
    if (def->getFlags() & ForbiddenFlags)
        return false;

    // 2. Reject internal ABI/pass-specific instructions (e.g., PUSH_ARG, POP_ARG)
    if (def->getTier() == MirInstructionTier::PassInternal)
        return false;

    // 3. Reject system-level instructions (e.g., SYSCALL, HALT)
    if (def->getCategory() == MirInstructionCategory::MirCat_System)
        return false;

    return true;
}

bool isInRange(int64_t val, int64_t min, int64_t max) { return val >= min && val <= max; }

bool isInRangeU(uint64_t val, uint64_t min, uint64_t max) { return val >= min && val <= max; }

bool isValidShiftAmount(const MirOperand *operand, uint32_t bitWidth)
{
    const FlexInt *imm = extractFlexInt(operand);
    if (!imm || imm->isNeg())
        return false;

    uint64_t shift = imm->getU64();
    return shift < static_cast<uint64_t>(bitWidth);
}

bool isAlignedDisplacement(const MirOperand *disp, uint32_t byteAlignment)
{
    if (byteAlignment == 0 || (byteAlignment & (byteAlignment - 1)) != 0)
        return false; // Alignment must be a power of 2

    const FlexInt *imm = extractFlexInt(disp);
    if (!imm)
        return false;

    uint64_t val = imm->getU64();
    return (val & (byteAlignment - 1)) == 0;
}

bool isPowerOfTwo(uint64_t val) { return val != 0 && (val & (val - 1)) == 0; }

bool isPowerOfTwoPlusOne(uint64_t val)
{
    if (val <= 1)
        return false;
    return isPowerOfTwo(val - 1);
}

bool isPowerOfTwoMinusOne(uint64_t val)
{
    if (val == 0 || val == UINT64_MAX)
        return false;
    return isPowerOfTwo(val + 1);
}

bool isAllOnes(const MirOperand *operand)
{
    const FlexInt *imm = extractFlexInt(operand);
    if (!imm)
        return false;

    size_t bits = imm->getBitSize();
    if (bits == 0)
        return false;

    uint64_t val = imm->getU64();
    if (bits >= 64)
        return val == UINT64_MAX;

    uint64_t mask = (1ULL << bits) - 1ULL;
    return (val & mask) == mask;
}

bool isContiguousMask(uint64_t val, uint32_t &maskOffset, uint32_t &maskLength)
{
    if (val == 0)
    {
        maskOffset = 0;
        maskLength = 0;
        return false;
    }

    uint32_t tz = static_cast<uint32_t>(std::countr_zero(val));
    uint64_t shifted = val >> tz;
    uint32_t len = static_cast<uint32_t>(std::countr_one(shifted));

    if ((shifted >> len) == 0)
    {
        maskOffset = tz;
        maskLength = len;
        return true;
    }

    return false;
}

bool isIntZero(const MirOperand *operand)
{
    const FlexInt *imm = extractFlexInt(operand);
    return imm ? imm->isZero() : false;
}

bool isIntNegative(const MirOperand *operand)
{
    const FlexInt *imm = extractFlexInt(operand);
    return imm ? imm->isNeg() : false;
}

bool isIntPositive(const MirOperand *operand)
{
    const FlexInt *imm = extractFlexInt(operand);
    return imm ? !imm->isNeg() : false;
}

/**
 * Proves the bits above bitWidth are zero either from the constant value or from the defining
 * instruction (ZEXT, a masking AND, or a SHR that leaves at most bitWidth significant bits).
 */
bool isZeroExtendedFrom(const MirOperand *operand, uint32_t bitWidth, const MirFunctionRegisterInfo *regInfo)
{
    if (!operand)
        return false;

    // Direct constant evaluation
    if (const FlexInt *imm = extractFlexInt(operand))
    {
        if (bitWidth >= 64)
            return true;
        uint64_t val = imm->getU64();
        uint64_t upperMask = ~((1ULL << bitWidth) - 1ULL);
        return (val & upperMask) == 0;
    }

    if (!regInfo)
        return false;

    MirInstruction *def = getDefiningInstr(operand, regInfo);
    if (!def)
        return false;

    switch (def->getOpCode())
    {
        // Explicit Zero-Extension: ZEXT dst, src
        case MirInstructionOpCode::ZEXT:
        {
            if (def->getOperands().size() > 1 && def->getOperand(1))
            {
                return getOperandBitWidth(def->getOperand(1)) <= bitWidth;
            }
            break;
        }

        // Bitwise AND: AND dst, lhs, mask
        case MirInstructionOpCode::AND:
        {
            if (def->getOperands().size() > 2)
            {
                if (const FlexInt *mask = extractFlexInt(def->getOperand(2)))
                {
                    if (bitWidth >= 64)
                        return true;
                    return (mask->getU64() >> bitWidth) == 0;
                }
            }
            break;
        }

        // Logical Shift Right: SHR dst, val, shamt
        case MirInstructionOpCode::SHR:
        {
            if (def->getOperands().size() > 2)
            {
                if (const FlexInt *shamt = extractFlexInt(def->getOperand(2)))
                {
                    uint32_t totalBits = getOperandBitWidth(operand);
                    uint64_t shiftVal = shamt->getU64();
                    if (shiftVal <= totalBits)
                    {
                        return (totalBits - shiftVal) <= bitWidth;
                    }
                }
            }
            break;
        }

        default:
            break;
    }

    return false;
}

/**
 * Proves the bits above bitWidth replicate the sign bit either from the constant value
 * (comparison against its own sign extension) or from a SEXT/SAR definition.
 */
bool isSignExtendedFrom(const MirOperand *operand, uint32_t bitWidth, const MirFunctionRegisterInfo *regInfo)
{
    if (!operand)
        return false;

    // Direct constant evaluation
    if (const FlexInt *imm = extractFlexInt(operand))
    {
        if (bitWidth >= 64)
            return true;

        int64_t val = imm->getI64();
        int64_t signExtended = (val << (64 - bitWidth)) >> (64 - bitWidth);
        return val == signExtended;
    }

    if (!regInfo)
        return false;

    MirInstruction *def = getDefiningInstr(operand, regInfo);
    if (!def)
        return false;

    switch (def->getOpCode())
    {
        // Explicit Sign-Extension: SEXT dst, src
        case MirInstructionOpCode::SEXT:
        {
            if (def->getOperands().size() > 1 && def->getOperand(1))
            {
                return getOperandBitWidth(def->getOperand(1)) <= bitWidth;
            }
            break;
        }

        // Arithmetic Shift Right: SAR dst, val, shamt
        case MirInstructionOpCode::SAR:
        {
            if (def->getOperands().size() > 2)
            {
                if (const FlexInt *shamt = extractFlexInt(def->getOperand(2)))
                {
                    uint32_t totalBits = getOperandBitWidth(operand);
                    uint64_t shiftVal = shamt->getU64();
                    if (shiftVal <= totalBits)
                    {
                        return (totalBits - shiftVal) <= bitWidth;
                    }
                }
            }
            break;
        }

        default:
            break;
    }

    return false;
}

/**
 * Checks that every user of a register discards the bits above liveBits, either because the
 * register is dead, consumers TRUNC to a narrow type, or consumers AND with a low mask.
 */
bool areHighBitsIgnored(const MirOperand *operand, uint32_t liveBits, const MirFunctionRegisterInfo *regInfo)
{
    if (!operand || liveBits == 0)
        return false;

    uint32_t totalBits = getOperandBitWidth(operand);
    if (liveBits >= totalBits)
        return false;

    if (!regInfo || operand->getType() != MirOperandType::Register)
        return false;

    const auto *reg = static_cast<const MirRegister *>(operand);
    auto usesOpt = regInfo->getUses(reg->getRegId());
    if (!usesOpt || usesOpt->empty())
        return true; // Dead register; high bits are completely unread

    // Check if every consumer instruction truncates or masks away high bits
    for (const auto &use : *usesOpt)
    {
        MirInstruction *userInst = use.m_userInst;
        if (!userInst)
            return false;

        switch (userInst->getOpCode())
        {
            case MirInstructionOpCode::TRUNC:
            {
                // Consumed by TRUNC; check if target type is <= liveBits
                if (userInst->getOperand(0) && getOperandBitWidth(userInst->getOperand(0)) <= liveBits)
                    continue;
                return false;
            }

            case MirInstructionOpCode::AND:
            {
                // Consumed by AND; check if mask zeros all bits above liveBits
                if (userInst->getOperands().size() > 2)
                {
                    if (const FlexInt *mask = extractFlexInt(userInst->getOperand(2)))
                    {
                        if ((mask->getU64() >> liveBits) == 0)
                            continue;
                    }
                }
                return false;
            }

            default:
                return false; // Unknown or full-width user
        }
    }

    return true;
}

bool isFloatZero(const MirOperand *operand)
{
    const FlexFloat *flt = extractFlexFloat(operand);
    return flt ? flt->isZero() : false;
}

bool isFloatNegative(const MirOperand *operand)
{
    const FlexFloat *flt = extractFlexFloat(operand);
    return flt ? flt->isNeg() : false;
}

bool isFloatPositive(const MirOperand *operand)
{
    const FlexFloat *flt = extractFlexFloat(operand);
    return flt ? flt->isPositive() : false;
}

bool isValidScaleFactor(int64_t scale) { return scale == 1 || scale == 2 || scale == 4 || scale == 8; }

bool isFrameIndex(const MirOperand *operand)
{
    if (!operand || operand->getType() != MirOperandType::Reference)
        return false;

    const auto *ref = static_cast<const MirReference *>(operand);
    return ref->isStackFrameObject();
}

} // namespace Predicates