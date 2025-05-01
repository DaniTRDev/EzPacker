#ifndef EZPACKER_ZYDISTRANSLATOR_H
#define EZPACKER_ZYDISTRANSLATOR_H

#include "Decoder/IDecodedInstructionParser.h"
#include "EzLifterCommon.h"

/**
 * This file's merely purpose is to translate from Zydis' instruction types into DecodedInstructionType.
 */

namespace translator
{
inline DecodedInstructionType translateZydisMnemonic(std::shared_ptr<ZydisDecodedInstruction> instr)
{
    if (!instr)
        return DecodedInstructionType::Invalid;

    switch (instr->mnemonic)
    {
    case ZYDIS_MNEMONIC_ADD:
        return DecodedInstructionType::Add;
    case ZYDIS_MNEMONIC_AND:
        return DecodedInstructionType::And;
    case ZYDIS_MNEMONIC_CALL:
        return DecodedInstructionType::Call;
    case ZYDIS_MNEMONIC_CMP:
        return DecodedInstructionType::Compare;
    case ZYDIS_MNEMONIC_DIV:
        return DecodedInstructionType::Divide;
    case ZYDIS_MNEMONIC_IMUL:
    case ZYDIS_MNEMONIC_MUL:
        return DecodedInstructionType::Multiply;
    case ZYDIS_MNEMONIC_JMP:
        return DecodedInstructionType::Jump;
    case ZYDIS_MNEMONIC_JLE:
    case ZYDIS_MNEMONIC_JL:
    case ZYDIS_MNEMONIC_JB:
    case ZYDIS_MNEMONIC_JBE:
    case ZYDIS_MNEMONIC_JS:
    case ZYDIS_MNEMONIC_JNS:
    case ZYDIS_MNEMONIC_JO:
    case ZYDIS_MNEMONIC_JNO:
    case ZYDIS_MNEMONIC_JP:
    case ZYDIS_MNEMONIC_JNP:
    case ZYDIS_MNEMONIC_JRCXZ:
    case ZYDIS_MNEMONIC_LOOP:
    case ZYDIS_MNEMONIC_LOOPE:
    case ZYDIS_MNEMONIC_LOOPNE:
        return DecodedInstructionType::Branch;
    case ZYDIS_MNEMONIC_LEA:
        return DecodedInstructionType::LoadEffectiveAddress;
    case ZYDIS_MNEMONIC_MOV:
        return DecodedInstructionType::_Mov;
    case ZYDIS_MNEMONIC_MOVZX:
    case ZYDIS_MNEMONIC_MOVSX:
        return DecodedInstructionType::SignExtend;
    case ZYDIS_MNEMONIC_NOP:
        return DecodedInstructionType::Nop;
    case ZYDIS_MNEMONIC_NOT:
        return DecodedInstructionType::Not;
    case ZYDIS_MNEMONIC_OR:
        return DecodedInstructionType::Or;
    case ZYDIS_MNEMONIC_POP:
        return DecodedInstructionType::Pop;
    case ZYDIS_MNEMONIC_PREFETCHNTA:
    case ZYDIS_MNEMONIC_PREFETCHT0:
    case ZYDIS_MNEMONIC_PREFETCHT1:
    case ZYDIS_MNEMONIC_PREFETCHT2:
        return DecodedInstructionType::Prefetch;
    case ZYDIS_MNEMONIC_PUSH:
        return DecodedInstructionType::Push;
    case ZYDIS_MNEMONIC_RET:
        return DecodedInstructionType::Return;
    case ZYDIS_MNEMONIC_ROL:
        return DecodedInstructionType::RotateLeft;
    case ZYDIS_MNEMONIC_ROR:
        return DecodedInstructionType::RotateRight;
    case ZYDIS_MNEMONIC_SAR:
    case ZYDIS_MNEMONIC_SHR:
        return DecodedInstructionType::ShiftRight;
    case ZYDIS_MNEMONIC_SHL:
        return DecodedInstructionType::ShiftLeft;
    case ZYDIS_MNEMONIC_STC:
    case ZYDIS_MNEMONIC_CLC:
    case ZYDIS_MNEMONIC_CLD:
    case ZYDIS_MNEMONIC_STD:
    case ZYDIS_MNEMONIC_STI:
    case ZYDIS_MNEMONIC_CLI:
        return DecodedInstructionType::SetFlags;
    case ZYDIS_MNEMONIC_SUB:
        return DecodedInstructionType::Subtract;
    case ZYDIS_MNEMONIC_SYSCALL:
    case ZYDIS_MNEMONIC_SYSENTER:
    case ZYDIS_MNEMONIC_INT:
    case ZYDIS_MNEMONIC_INT3:
        return DecodedInstructionType::SysCall;
    case ZYDIS_MNEMONIC_TEST:
        return DecodedInstructionType::Test;
    case ZYDIS_MNEMONIC_XOR:
        return DecodedInstructionType::Xor;
    case ZYDIS_MNEMONIC_XCHG:
        return DecodedInstructionType::Exchange; // Optional: consider XCHG special?

    default:
        return DecodedInstructionType::Invalid;
    }
}
inline ConditionType translateZydisCondition(std::shared_ptr<ZydisDecodedInstruction> instr)
{
    // TODO: Add support for Conditional moves / sets.
    if (!instr)
        return ConditionType::Invalid;

    switch (instr->mnemonic) {
    case ZYDIS_MNEMONIC_JZ:
        return ConditionType::Equal;
    case ZYDIS_MNEMONIC_JNZ:
        return ConditionType::NotEqual;
    case ZYDIS_MNEMONIC_JS:
        return ConditionType::Signed;
    case ZYDIS_MNEMONIC_JNS:
        return ConditionType::NotSigned;
    case ZYDIS_MNEMONIC_JO:
        return ConditionType::Overflow;
    case ZYDIS_MNEMONIC_JNO:
        return ConditionType::NotOverflow;
    case ZYDIS_MNEMONIC_JB:
        return ConditionType::Less;
    case ZYDIS_MNEMONIC_JNB:
        return ConditionType::GreaterEqual;
    case ZYDIS_MNEMONIC_JL:
        return ConditionType::Less;
    case ZYDIS_MNEMONIC_JLE:
        return ConditionType::LessEqual;
    case ZYDIS_MNEMONIC_JNLE:
        return ConditionType::Greater;
    case ZYDIS_MNEMONIC_JNL:
        return ConditionType::GreaterEqual;
    case ZYDIS_MNEMONIC_JP:
        return ConditionType::Parity;
    case ZYDIS_MNEMONIC_JNP:
        return ConditionType::NotParity;
    default:
        return ConditionType::Invalid;
    }
}
} // namespace translator

#endif // EZPACKER_ZYDISTRANSLATOR_H
