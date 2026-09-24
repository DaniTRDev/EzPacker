#include "Encoding/X86_64InstructionEncoder.h"

#include <cstdint>

namespace EzTargets::X86_64
{

namespace
{

// Sentinel meaning "no register" in base/index positions.
constexpr uint8_t NoReg = 0xFF;

// Accumulated REX prefix bits for a single instruction encoding.
struct Rex
{
    bool w{ false }; ///< REX.W: 64-bit operand size.
    bool r{ false }; ///< REX.R: extension of the ModR/M.reg field.
    bool x{ false }; ///< REX.X: extension of the SIB.index field.
    bool b{ false }; ///< REX.B: extension of the ModR/M.rm / SIB.base field.

    // True when any REX bit is set, meaning a prefix byte must be emitted.
    bool isNeeded() const { return w || r || x || b; }
    // Packs the REX bits into the 0x40-based prefix byte.
    uint8_t encode() const { return InstructionEncoder::rexByte(w, r, x, b); }
};

uint8_t low3(uint8_t enc) { return static_cast<uint8_t>(enc & 0x07u); }
uint8_t low4(uint8_t enc) { return static_cast<uint8_t>(enc & 0x0Fu); }
bool extBit(uint8_t enc) { return ((enc >> 3) & 0x01u) != 0; }

// Converts an index scale factor (1/2/4/8) into its 2-bit SIB encoding (0..3).
uint8_t scaleToPower(uint8_t scale)
{
    switch (scale)
    {
        case 1:
            return 0;
        case 2:
            return 1;
        case 4:
            return 2;
        case 8:
            return 3;
        default:
            return 0;
    }
}

// Appends the low `bytes` bytes of value in little-endian order.
void emitImm(std::vector<uint8_t> &out, int64_t value, uint8_t bytes)
{
    for (uint8_t i = 0; i < bytes; ++i)
    {
        out.push_back(static_cast<uint8_t>((static_cast<uint64_t>(value) >> (i * 8)) & 0xFFu));
    }
}

// ModR/M + SIB + displacement bytes for one operand, plus relocation metadata.
struct ModRMPlan
{
    std::vector<uint8_t> bytes;    ///< Encoded addressing bytes.
    bool hasReloc{ false };        ///< True when a displacement field needs a relocation.
    size_t relocOffsetWithin{ 0 }; ///< Offset of the relocatable field within bytes.
    uint8_t relocBits{ 32 };       ///< Width in bits of that field.
};

// Builds ModR/M bytes for a register-direct r/m operand (mod = 3).
ModRMPlan buildRegisterModRM(uint8_t regField, uint8_t rmEncoding, Rex &rex)
{
    ModRMPlan plan;
    rex.b = extBit(rmEncoding);
    plan.bytes.push_back(InstructionEncoder::encodeModRM(3, regField, low3(rmEncoding)));
    return plan;
}

// Builds the full ModR/M + SIB + displacement sequence for a memory operand,
// selecting the shortest legal displacement encoding for the addressing form.
ModRMPlan buildMemoryModRM(uint8_t regField, const EncMemory &mem, Rex &rex)
{
    ModRMPlan plan;

    if (mem.m_ripRel)
    {
        // [rip + disp32]: mod=00, rm=101, optionally recording a rel32 fixup.
        plan.bytes.push_back(InstructionEncoder::encodeModRM(0, regField, 5));
        if (mem.m_needsReloc)
        {
            plan.hasReloc = true;
            plan.relocOffsetWithin = plan.bytes.size();
            plan.relocBits = 32;
        }
        emitImm(plan.bytes, mem.m_disp, 4);
        return plan;
    }

    if (mem.m_base == NoReg && mem.m_index == NoReg)
    {
        // Absolute disp32 (mod=00, SIB with base=101 and no index).
        plan.bytes.push_back(InstructionEncoder::encodeModRM(0, regField, 4));
        plan.bytes.push_back(InstructionEncoder::encodeSIB(0, 4, 5));
        emitImm(plan.bytes, mem.m_disp, 4);
        return plan;
    }

    if (mem.m_index != NoReg)
    {
        rex.x = extBit(mem.m_index);
        if (mem.m_base == NoReg)
        {
            // Index-only addressing requires a disp32 base of 5.
            plan.bytes.push_back(InstructionEncoder::encodeModRM(0, regField, 4));
            plan.bytes.push_back(InstructionEncoder::encodeSIB(scaleToPower(mem.m_scale), low3(mem.m_index), 5));
            emitImm(plan.bytes, mem.m_disp, 4);
            return plan;
        }

        uint8_t baseLow = low3(mem.m_base);
        rex.b = extBit(mem.m_base);

        // Choose mod/displacement form: no disp when possible, disp8 when it fits, else disp32.
        uint8_t mod = 0;
        bool emitDisp8 = false;
        bool emitDisp32 = false;
        if (mem.m_disp == 0)
        {
            if (baseLow == 5)
            {
                mod = 1;
                emitDisp8 = true;
            }
        }
        else if (mem.m_disp >= -128 && mem.m_disp <= 127)
        {
            mod = 1;
            emitDisp8 = true;
        }
        else
        {
            mod = 2;
            emitDisp32 = true;
        }

        plan.bytes.push_back(InstructionEncoder::encodeModRM(mod, regField, 4));
        plan.bytes.push_back(InstructionEncoder::encodeSIB(scaleToPower(mem.m_scale), low3(mem.m_index), baseLow));
        if (emitDisp8)
        {
            emitImm(plan.bytes, mem.m_disp, 1);
        }
        else if (emitDisp32)
        {
            emitImm(plan.bytes, mem.m_disp, 4);
        }
        return plan;
    }

    uint8_t baseLow = low3(mem.m_base);
    rex.b = extBit(mem.m_base);
    bool needSib = (baseLow == 4); // rsp/r12 as base always needs a SIB byte.

    // Choose mod/displacement form: no disp when possible, disp8 when it fits, else disp32.
    uint8_t mod = 0;
    bool emitDisp8 = false;
    bool emitDisp32 = false;
    if (mem.m_disp == 0)
    {
        if (baseLow == 5)
        {
            mod = 1;
            emitDisp8 = true;
        }
    }
    else if (mem.m_disp >= -128 && mem.m_disp <= 127)
    {
        mod = 1;
        emitDisp8 = true;
    }
    else
    {
        mod = 2;
        emitDisp32 = true;
    }

    plan.bytes.push_back(InstructionEncoder::encodeModRM(mod, regField, needSib ? 4 : baseLow));
    if (needSib)
    {
        plan.bytes.push_back(InstructionEncoder::encodeSIB(0, 4, baseLow));
    }
    if (emitDisp8)
    {
        emitImm(plan.bytes, mem.m_disp, 1);
    }
    else if (emitDisp32)
    {
        emitImm(plan.bytes, mem.m_disp, 4);
    }
    return plan;
}

// Returns the binding that assigns an operand to the given slot, or nullptr if unbound.
const EncOperandBinding *findBinding(const EncodingDesc &desc, EncSlotKind slot)
{
    for (uint8_t i = 0; i < desc.m_operandCount; ++i)
    {
        if (desc.m_operands[i].m_slot == slot)
        {
            return &desc.m_operands[i];
        }
    }
    return nullptr;
}

// Resolves the operand bound to the given slot, or nullptr when absent/out of range.
const ResolvedOperand *resolve(const EncodingDesc &desc, std::span<const ResolvedOperand> operands, EncSlotKind slot)
{
    const EncOperandBinding *binding = findBinding(desc, slot);
    if (!binding)
    {
        return nullptr;
    }
    if (binding->m_operandIndex >= operands.size())
    {
        return nullptr;
    }
    return &operands[binding->m_operandIndex];
}

// Determines the operation size: the explicit size operand if any, otherwise the
// first register operand, defaulting to 8 bytes.
uint8_t resolveSize(const EncodingDesc &desc, std::span<const ResolvedOperand> operands)
{
    if (desc.m_sizeOperand != 0xFF && desc.m_sizeOperand < operands.size())
    {
        return operands[desc.m_sizeOperand].m_sizeBytes;
    }

    for (uint8_t i = 0; i < desc.m_operandCount; ++i)
    {
        const EncOperandBinding &binding = desc.m_operands[i];
        if ((binding.m_slot == EncSlotKind::Reg || binding.m_slot == EncSlotKind::RmReg) &&
            binding.m_operandIndex < operands.size() &&
            operands[binding.m_operandIndex].m_kind == ResolvedOperand::Kind::Register)
        {
            return operands[binding.m_operandIndex].m_sizeBytes;
        }
    }
    return 8;
}

// Emits legacy/mandatory prefixes in the architecturally required order, adding the
// 0x66 operand-size prefix when a 16-bit operation is requested.
void emitLegacyPrefixes(std::vector<uint8_t> &out, uint8_t prefixes, bool size16)
{
    if (prefixes & EncPrefixF0)
    {
        out.push_back(0xF0);
    }
    if (prefixes & EncPrefixF2)
    {
        out.push_back(0xF2);
    }
    if (prefixes & EncPrefixF3)
    {
        out.push_back(0xF3);
    }
    if ((prefixes & EncPrefix66) || size16)
    {
        out.push_back(0x66);
    }
    if (prefixes & EncPrefix67)
    {
        out.push_back(0x67);
    }
}

// Copies the descriptor's fixed opcode bytes into the output.
void emitRawOpcode(std::vector<uint8_t> &out, const uint8_t *opcode, uint8_t len)
{
    for (uint8_t i = 0; i < len; ++i)
    {
        out.push_back(opcode[i]);
    }
}

// Applies the descriptor's REX.W policy: 0 = never, 1 = always, 2 = when the operand is 8 bytes.
bool rexWanted(uint8_t rexWPolicy, uint8_t sizeBytes)
{
    if (rexWPolicy == 1)
    {
        return true;
    }
    if (rexWPolicy == 2)
    {
        return sizeBytes == 8;
    }
    return false;
}

// Encodes a ModR/M form (Rr/Rm/Mr/Test/Movzx/Movsx/Lea/Sse/Cvt/ImulRR).
bool encodeModRMForm(const EncodingDesc &desc,
                     std::span<const ResolvedOperand> operands,
                     std::vector<uint8_t> &out,
                     EzCodeEmitter::EncodeResult &result)
{
    const ResolvedOperand *regOp = resolve(desc, operands, EncSlotKind::Reg);
    const ResolvedOperand *rmOp = resolve(desc, operands, EncSlotKind::RmReg);
    if (!rmOp)
    {
        rmOp = resolve(desc, operands, EncSlotKind::RmMem);
    }
    if (!regOp || !rmOp || regOp->m_kind != ResolvedOperand::Kind::Register ||
        (rmOp->m_kind != ResolvedOperand::Kind::Register && rmOp->m_kind != ResolvedOperand::Kind::Memory))
    {
        return false;
    }

    // rmIsMemory distinguishes the memory form from the register-direct form.
    const EncOperandBinding *rmBinding = findBinding(desc, EncSlotKind::RmMem);
    const bool rmIsMemory = rmBinding && rmBinding->m_operandIndex == static_cast<uint8_t>(rmOp - operands.data());
    // Floating-point register with an SSE variant switches to the SSE opcode stream.
    const bool useSse = desc.m_hasSseVariant && regOp->m_isFpr;

    uint8_t sizeBytes = resolveSize(desc, operands);
    Rex rex;
    rex.r = extBit(regOp->m_reg);
    rex.w = useSse ? false : rexWanted(desc.m_rexW, sizeBytes);

    ModRMPlan plan;
    if (rmIsMemory)
    {
        plan = buildMemoryModRM(low3(regOp->m_reg), rmOp->m_mem, rex);
    }
    else
    {
        plan = buildRegisterModRM(low3(regOp->m_reg), rmOp->m_reg, rex);
    }

    // Byte registers SPL/BPL/SIL/DIL (encoding low nibble >= 4) need a REX prefix to be addressable.
    bool forceRex =
            (sizeBytes == 1 && desc.m_byteRex && (low4(regOp->m_reg) >= 4 || (!rmIsMemory && low4(rmOp->m_reg) >= 4)));

    emitLegacyPrefixes(out, useSse ? desc.m_ssePrefixes : desc.m_prefixes, !useSse && sizeBytes == 2);
    if (rex.isNeeded() || forceRex)
    {
        out.push_back(rex.encode());
    }

    const uint8_t *opcode = useSse ? desc.m_sseOpcode : desc.m_opcode;
    uint8_t opcodeLen = useSse ? desc.m_sseOpcodeLen : desc.m_opcodeLen;
    // Byte forms of ALU group opcodes are one less than their word/dword counterparts (e.g. 0x89 -> 0x88).
    bool byteMinusOne = !useSse && sizeBytes == 1;
    for (uint8_t i = 0; i < opcodeLen; ++i)
    {
        uint8_t byte = opcode[i];
        if (byteMinusOne && i + 1 == opcodeLen)
        {
            byte = static_cast<uint8_t>(byte - 1);
        }
        out.push_back(byte);
    }

    // Append the addressing bytes, rebasing any relocation offset onto the full instruction.
    size_t planStart = out.size();
    out.insert(out.end(), plan.bytes.begin(), plan.bytes.end());
    if (plan.hasReloc)
    {
        result.m_hasReloc = true;
        result.m_relocOffset = planStart + plan.relocOffsetWithin;
        result.m_relocBits = plan.relocBits;
    }
    return true;
}

// Encodes an ALU-style reg, imm form with 0x80/0x81/0x83 selection.
bool encodeAluImmForm(const EncodingDesc &desc, std::span<const ResolvedOperand> operands, std::vector<uint8_t> &out)
{
    const ResolvedOperand *rmOp = resolve(desc, operands, EncSlotKind::RmReg);
    const ResolvedOperand *immOp = resolve(desc, operands, EncSlotKind::Imm8Signed);
    if (!immOp)
    {
        immOp = resolve(desc, operands, EncSlotKind::Imm32);
    }
    if (!rmOp || !immOp || rmOp->m_kind != ResolvedOperand::Kind::Register)
    {
        return false;
    }

    uint8_t sizeBytes = resolveSize(desc, operands);
    int64_t imm = immOp->m_imm;
    bool fitsImm8 = (imm >= -128 && imm <= 127); // Sign-extended imm8 avoids a full-width immediate.

    Rex rex;
    rex.b = extBit(rmOp->m_reg);
    rex.w = rexWanted(desc.m_rexW, sizeBytes);

    std::vector<uint8_t> modrm;
    modrm.push_back(InstructionEncoder::encodeModRM(3, desc.m_digit, low3(rmOp->m_reg)));

    bool forceRex = (sizeBytes == 1 && desc.m_byteRex && low4(rmOp->m_reg) >= 4);

    emitLegacyPrefixes(out, desc.m_prefixes, sizeBytes == 2);
    if (rex.isNeeded() || forceRex)
    {
        out.push_back(rex.encode());
    }

    if (sizeBytes == 1)
    {
        out.push_back(0x80);
    }
    else if (fitsImm8)
    {
        out.push_back(0x83);
    }
    else
    {
        out.push_back(0x81);
    }
    out.insert(out.end(), modrm.begin(), modrm.end());

    if (sizeBytes == 1 || fitsImm8)
    {
        emitImm(out, imm, 1);
    }
    else if (sizeBytes == 2)
    {
        emitImm(out, imm, 2);
    }
    else
    {
        emitImm(out, imm, 4);
    }
    return true;
}

// Encodes the IMUL reg, r/m, imm form (0x69/0x6B selection).
bool encodeImulImmForm(const EncodingDesc &desc, std::span<const ResolvedOperand> operands, std::vector<uint8_t> &out)
{
    const ResolvedOperand *regOp = resolve(desc, operands, EncSlotKind::Reg);
    const ResolvedOperand *rmOp = resolve(desc, operands, EncSlotKind::RmReg);
    const ResolvedOperand *immOp = resolve(desc, operands, EncSlotKind::Imm8Signed);
    if (!immOp)
    {
        immOp = resolve(desc, operands, EncSlotKind::Imm32);
    }
    if (!regOp || !rmOp || !immOp || regOp->m_kind != ResolvedOperand::Kind::Register ||
        rmOp->m_kind != ResolvedOperand::Kind::Register)
    {
        return false;
    }

    uint8_t sizeBytes = resolveSize(desc, operands);
    int64_t imm = immOp->m_imm;
    bool fitsImm8 = (imm >= -128 && imm <= 127);

    Rex rex;
    rex.r = extBit(regOp->m_reg);
    rex.b = extBit(rmOp->m_reg);
    rex.w = rexWanted(desc.m_rexW, sizeBytes);

    emitLegacyPrefixes(out, desc.m_prefixes, sizeBytes == 2);
    if (rex.isNeeded())
    {
        out.push_back(rex.encode());
    }

    out.push_back(fitsImm8 ? 0x6B : 0x69);
    out.push_back(InstructionEncoder::encodeModRM(3, low3(regOp->m_reg), low3(rmOp->m_reg)));

    if (fitsImm8)
    {
        emitImm(out, imm, 1);
    }
    else if (sizeBytes == 2)
    {
        emitImm(out, imm, 2);
    }
    else
    {
        emitImm(out, imm, 4);
    }
    return true;
}

// Encodes the MOV r, imm family, including the 64-bit MOVABS special case.
bool encodeMovImmForm(const EncodingDesc &desc, std::span<const ResolvedOperand> operands, std::vector<uint8_t> &out)
{
    const ResolvedOperand *regOp = resolve(desc, operands, EncSlotKind::Reg);
    const ResolvedOperand *immOp = resolve(desc, operands, EncSlotKind::Imm8);
    if (!immOp)
        immOp = resolve(desc, operands, EncSlotKind::Imm16);
    if (!immOp)
        immOp = resolve(desc, operands, EncSlotKind::Imm32);
    if (!immOp)
        immOp = resolve(desc, operands, EncSlotKind::Imm64);
    if (!regOp || !immOp || regOp->m_kind != ResolvedOperand::Kind::Register)
    {
        return false;
    }

    uint8_t sizeBytes = resolveSize(desc, operands);
    int64_t imm = immOp->m_imm;
    uint8_t rd = low3(regOp->m_reg);
    bool extended = extBit(regOp->m_reg);

    if (sizeBytes == 8)
    {
        if (imm >= -2147483648LL && imm <= 2147483647LL)
        {
            // Fits in a sign-extended imm32: use the shorter C7 /0 form.
            out.push_back(InstructionEncoder::rexByte(true, false, false, extended));
            out.push_back(0xC7);
            out.push_back(InstructionEncoder::encodeModRM(3, 0, rd));
            emitImm(out, imm, 4);
        }
        else
        {
            // Full 64-bit immediate: MOVABS B8+rd with an imm64.
            out.push_back(InstructionEncoder::rexByte(true, false, false, extended));
            out.push_back(static_cast<uint8_t>(0xB8 + rd));
            emitImm(out, imm, 8);
        }
        return true;
    }

    if (sizeBytes == 4)
    {
        if (extended)
        {
            out.push_back(InstructionEncoder::rexByte(false, false, false, true));
        }
        out.push_back(static_cast<uint8_t>(0xB8 + rd));
        emitImm(out, imm, 4);
        return true;
    }

    if (sizeBytes == 2)
    {
        out.push_back(0x66);
        if (extended)
        {
            out.push_back(InstructionEncoder::rexByte(false, false, false, true));
        }
        out.push_back(static_cast<uint8_t>(0xB8 + rd));
        emitImm(out, imm, 2);
        return true;
    }

    if (extended || low4(regOp->m_reg) >= 4)
    {
        out.push_back(InstructionEncoder::rexByte(false, false, false, extended));
    }
    out.push_back(static_cast<uint8_t>(0xB0 + rd));
    emitImm(out, imm, 1);
    return true;
}

// Encodes a single-register F6/F7 group instruction (NEG/NOT/DIV/IDIV).
bool encodeUnaryForm(const EncodingDesc &desc, std::span<const ResolvedOperand> operands, std::vector<uint8_t> &out)
{
    const ResolvedOperand *regOp = resolve(desc, operands, EncSlotKind::RmReg);
    if (!regOp || regOp->m_kind != ResolvedOperand::Kind::Register)
    {
        return false;
    }

    uint8_t sizeBytes = resolveSize(desc, operands);
    Rex rex;
    rex.b = extBit(regOp->m_reg);
    rex.w = rexWanted(desc.m_rexW, sizeBytes);

    bool forceRex = (sizeBytes == 1 && desc.m_byteRex && low4(regOp->m_reg) >= 4);

    emitLegacyPrefixes(out, desc.m_prefixes, sizeBytes == 2);
    if (rex.isNeeded() || forceRex)
    {
        out.push_back(rex.encode());
    }

    out.push_back(sizeBytes == 1 ? 0xF6 : 0xF7);
    out.push_back(InstructionEncoder::encodeModRM(3, desc.m_digit, low3(regOp->m_reg)));
    return true;
}

// Encodes shift instructions (immediate or CL variants).
bool encodeShiftForm(const EncodingDesc &desc, std::span<const ResolvedOperand> operands, std::vector<uint8_t> &out)
{
    const ResolvedOperand *regOp = resolve(desc, operands, EncSlotKind::RmReg);
    if (!regOp || regOp->m_kind != ResolvedOperand::Kind::Register)
    {
        return false;
    }

    uint8_t sizeBytes = resolveSize(desc, operands);
    Rex rex;
    rex.b = extBit(regOp->m_reg);
    rex.w = rexWanted(desc.m_rexW, sizeBytes);

    bool forceRex = (sizeBytes == 1 && desc.m_byteRex && low4(regOp->m_reg) >= 4);

    emitLegacyPrefixes(out, desc.m_prefixes, sizeBytes == 2);
    if (rex.isNeeded() || forceRex)
    {
        out.push_back(rex.encode());
    }

    if (desc.m_shiftByCL)
    {
        out.push_back(sizeBytes == 1 ? 0xD2 : 0xD3);
        out.push_back(InstructionEncoder::encodeModRM(3, desc.m_digit, low3(regOp->m_reg)));
        return true;
    }

    const ResolvedOperand *immOp = resolve(desc, operands, EncSlotKind::Imm8);
    if (!immOp)
    {
        immOp = resolve(desc, operands, EncSlotKind::Imm8Signed);
    }
    if (!immOp)
    {
        return false;
    }

    uint8_t amount = static_cast<uint8_t>(immOp->m_imm);
    if (amount == 1)
    {
        out.push_back(sizeBytes == 1 ? 0xD0 : 0xD1);
        out.push_back(InstructionEncoder::encodeModRM(3, desc.m_digit, low3(regOp->m_reg)));
    }
    else
    {
        out.push_back(sizeBytes == 1 ? 0xC0 : 0xC1);
        out.push_back(InstructionEncoder::encodeModRM(3, desc.m_digit, low3(regOp->m_reg)));
        out.push_back(amount);
    }
    return true;
}

// Encodes a SETcc instruction.
bool encodeSetccForm(const EncodingDesc &desc, std::span<const ResolvedOperand> operands, std::vector<uint8_t> &out)
{
    const ResolvedOperand *regOp = resolve(desc, operands, EncSlotKind::RmReg);
    if (!regOp || regOp->m_kind != ResolvedOperand::Kind::Register)
    {
        return false;
    }

    // SETcc targets byte registers, so SPL/BPL/SIL/DIL require a REX prefix.
    bool forceRex = (low4(regOp->m_reg) >= 4);
    if (forceRex)
    {
        out.push_back(InstructionEncoder::rexByte(false, false, false, extBit(regOp->m_reg)));
    }

    out.push_back(0x0F);
    out.push_back(static_cast<uint8_t>(0x90 | (desc.m_condCode & 0x0F)));
    out.push_back(InstructionEncoder::encodeModRM(3, 0, low3(regOp->m_reg)));
    return true;
}

// Encodes a Jcc/JMP/CALL relative branch, reporting the rel32 relocation offset.
bool encodeBranchForm(const EncodingDesc &desc,
                      std::span<const ResolvedOperand> operands,
                      std::vector<uint8_t> &out,
                      EzCodeEmitter::EncodeResult &result,
                      bool isJcc,
                      bool allowRegister)
{
    const ResolvedOperand *op = resolve(desc, operands, EncSlotKind::Rel32);
    if (!op)
    {
        op = resolve(desc, operands, EncSlotKind::Reg);
    }
    if (!op)
    {
        // Branch to an explicit immediate displacement (no symbol).
        op = resolve(desc, operands, EncSlotKind::Imm32);
    }
    if (!op)
    {
        return false;
    }

    if (allowRegister && op->m_kind == ResolvedOperand::Kind::Register)
    {
        Rex rex;
        rex.b = extBit(op->m_reg);
        if (rex.isNeeded())
        {
            out.push_back(rex.encode());
        }
        out.push_back(0xFF);
        out.push_back(InstructionEncoder::encodeModRM(3, desc.m_digit, low3(op->m_reg)));
        return true;
    }

    emitRawOpcode(out, desc.m_opcode, desc.m_opcodeLen);
    if (isJcc && desc.m_opcodeLen > 0)
    {
        // Fold the condition code into the low nibble of the final opcode byte.
        out.back() = static_cast<uint8_t>(out.back() | (desc.m_condCode & 0x0F));
    }

    size_t relOffset = out.size();
    int64_t disp = (op->m_kind == ResolvedOperand::Kind::Immediate) ? op->m_imm : 0;
    emitImm(out, disp, 4);

    if (op->m_needsReloc)
    {
        result.m_hasReloc = true;
        result.m_relocOffset = relOffset;
        result.m_relocBits = 32;
        result.m_isBranch = true;
    }
    return true;
}

} // namespace

bool InstructionEncoder::encode(const EncodingDesc &desc,
                                std::span<const ResolvedOperand> operands,
                                std::vector<uint8_t> &out,
                                EzCodeEmitter::EncodeResult &result)
{
    result = {};
    std::vector<uint8_t> bytes; // Staged so `out` is left untouched on failure.

    bool ok = false;
    switch (desc.m_form)
    {
        case EncForm::Rr:
        case EncForm::Rm:
        case EncForm::Mr:
        case EncForm::Test:
        case EncForm::Movzx:
        case EncForm::Movsx:
        case EncForm::Lea:
        case EncForm::Sse:
        case EncForm::Cvt:
        case EncForm::ImulRR:
            ok = encodeModRMForm(desc, operands, bytes, result);
            break;

        case EncForm::Ri:
            ok = encodeAluImmForm(desc, operands, bytes);
            break;

        case EncForm::ImulRI:
            ok = encodeImulImmForm(desc, operands, bytes);
            break;

        case EncForm::MovRI:
            ok = encodeMovImmForm(desc, operands, bytes);
            break;

        case EncForm::Unary:
        case EncForm::Div:
            ok = encodeUnaryForm(desc, operands, bytes);
            break;

        case EncForm::Shift:
            ok = encodeShiftForm(desc, operands, bytes);
            break;

        case EncForm::Setcc:
            ok = encodeSetccForm(desc, operands, bytes);
            break;

        case EncForm::Jcc:
            ok = encodeBranchForm(desc, operands, bytes, result, /*isJcc=*/true, /*allowRegister=*/false);
            break;

        case EncForm::Jmp:
            ok = encodeBranchForm(desc, operands, bytes, result, /*isJcc=*/false, /*allowRegister=*/true);
            break;

        case EncForm::Call:
            ok = encodeBranchForm(desc, operands, bytes, result, /*isJcc=*/false, /*allowRegister=*/true);
            break;

        case EncForm::Push:
        case EncForm::Pop:
        {
            // Register push/pop: opcode base plus the low 3 bits of the register encoding.
            const ResolvedOperand *regOp = resolve(desc, operands, EncSlotKind::Reg);
            if (regOp && regOp->m_kind == ResolvedOperand::Kind::Register)
            {
                Rex rex;
                rex.b = extBit(regOp->m_reg);
                if (rex.isNeeded())
                {
                    bytes.push_back(rex.encode());
                }
                bytes.push_back(static_cast<uint8_t>(desc.m_opcode[0] + low3(regOp->m_reg)));
                ok = true;
            }
            break;
        }

        case EncForm::Ret:
        case EncForm::Nop:
        case EncForm::Syscall:
            emitRawOpcode(bytes, desc.m_opcode, desc.m_opcodeLen);
            ok = desc.m_opcodeLen > 0;
            break;

        case EncForm::None:
        default:
            ok = false;
            break;
    }

    if (!ok)
    {
        return false;
    }

    out.insert(out.end(), bytes.begin(), bytes.end());
    return true;
}

void InstructionEncoder::encodeRegisterMove(const ResolvedOperand &dst,
                                            const ResolvedOperand &src,
                                            std::vector<uint8_t> &out)
{
    if (dst.m_isFpr || src.m_isFpr)
    {
        Rex rex;
        rex.r = extBit(dst.m_reg);
        rex.b = extBit(src.m_reg);

        if (dst.m_sizeBytes == 16 || src.m_sizeBytes == 16)
        {
            // 128-bit vector move: MOVAPS 0F 28 /r with reg=dst and rm=src.
            if (rex.isNeeded())
            {
                out.push_back(rex.encode());
            }

            out.push_back(0x0F);
            out.push_back(0x28);
            out.push_back(InstructionEncoder::encodeModRM(3, low3(dst.m_reg), low3(src.m_reg)));
            return;
        }

        // Scalar SSE move: F3/F2 0F 10 /r with reg=dst and rm=src.
        out.push_back(dst.m_sizeBytes == 4 ? 0xF3 : 0xF2);

        if (rex.isNeeded())
        {
            out.push_back(rex.encode());
        }

        out.push_back(0x0F);
        out.push_back(0x10);
        out.push_back(InstructionEncoder::encodeModRM(3, low3(dst.m_reg), low3(src.m_reg)));
        return;
    }

    // General-purpose move: 0x88/0x89 with reg=src and rm=dst.
    Rex rex;
    rex.w = (dst.m_sizeBytes == 8);
    rex.r = extBit(src.m_reg);
    rex.b = extBit(dst.m_reg);

    bool forceRex = (dst.m_sizeBytes == 1 && (low4(dst.m_reg) >= 4 || low4(src.m_reg) >= 4));

    if (dst.m_sizeBytes == 2)
    {
        out.push_back(0x66);
    }
    if (rex.isNeeded() || forceRex)
    {
        out.push_back(rex.encode());
    }

    out.push_back(dst.m_sizeBytes == 1 ? 0x88 : 0x89);
    out.push_back(InstructionEncoder::encodeModRM(3, low3(src.m_reg), low3(dst.m_reg)));
}

void InstructionEncoder::emitJmpShort(std::vector<uint8_t> &out, int8_t disp)
{
    out.push_back(0xEB);
    out.push_back(static_cast<uint8_t>(disp));
}

void InstructionEncoder::emitJmpNear(std::vector<uint8_t> &out, int32_t disp)
{
    out.push_back(kNearJmpOpcode);
    emitImm(out, disp, 4);
}

void InstructionEncoder::emitJccShort(std::vector<uint8_t> &out, ConditionCode cc, int8_t disp)
{
    out.push_back(static_cast<uint8_t>(0x70 | (static_cast<uint8_t>(cc) & 0x0F)));
    out.push_back(static_cast<uint8_t>(disp));
}

void InstructionEncoder::emitJccNear(std::vector<uint8_t> &out, ConditionCode cc, int32_t disp)
{
    out.push_back(kNearJccPrefix);
    out.push_back(static_cast<uint8_t>(kNearJccBase | (static_cast<uint8_t>(cc) & 0x0F)));
    emitImm(out, disp, 4);
}

bool InstructionEncoder::classifyNearBranch(std::span<const uint8_t> bytes,
                                            size_t offset,
                                            size_t &dispOffset,
                                            size_t &instrLength)
{
    if (offset >= bytes.size())
    {
        return false;
    }

    const uint8_t op0 = bytes[offset];
    if (op0 == kNearJmpOpcode || op0 == kNearCallOpcode)
    {
        dispOffset = offset + 1;
        instrLength = 5;
        return true;
    }
    if (op0 == kNearJccPrefix && offset + 1 < bytes.size() && (bytes[offset + 1] & 0xF0) == kNearJccBase)
    {
        dispOffset = offset + 2;
        instrLength = 6;
        return true;
    }
    return false;
}

bool InstructionEncoder::writeDisp32(std::span<uint8_t> bytes, size_t offset, int32_t value)
{
    if (offset + 4 > bytes.size())
    {
        return false;
    }

    const uint32_t v = static_cast<uint32_t>(value);
    bytes[offset + 0] = static_cast<uint8_t>(v & 0xFFu);
    bytes[offset + 1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
    bytes[offset + 2] = static_cast<uint8_t>((v >> 16) & 0xFFu);
    bytes[offset + 3] = static_cast<uint8_t>((v >> 24) & 0xFFu);
    return true;
}

} // namespace EzTargets::X86_64
