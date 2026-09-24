#include "X86_64CodeEmitter.h"
#include "CodeSection.h"
#include "CodeEmitterContext.h"
#include "Operand/MirOperands.h"
#include "Operand/MirRegisterClass.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionStackFrame.h"
#include "Type/MirType.h"
#include "Encoding/X86_64InstructionEncoder.h"
#include <stdexcept>
#include <string>
#include <string_view>

namespace EzTargets::X86_64
{

namespace
{

/// Sentinel returned when a register cannot be mapped to a hardware encoding.
constexpr uint8_t InvalidRegister = 0xFF;

/// Returns true when a hardware encoding identifies an FPR/XMM register.
constexpr bool isFprEncoding(uint8_t encoding) { return encoding >= 16; }

/// Converts an FPR/XMM hardware encoding into its 0..15 register index.
constexpr uint8_t fprIndex(uint8_t encoding) { return static_cast<uint8_t>(encoding - 16); }

/// Hardware encoding of the frame-pointer register used for stack-slot addressing.
constexpr uint8_t FramePointerEncoding = 5; // RBP

/**
 * Returns the size in bytes of an operand, preferring its MIR type and falling back to a
 * machine-word default when the type is unknown.
 */
uint8_t operandSizeBytes(MirOperand *op, MirRegister *reg)
{
    (void)reg;
    if (op)
    {
        MirType *type = op->getMirType();
        if (type)
        {
            size_t bits = type->getTotalSizeInBits();
            if (bits > 0)
            {
                return static_cast<uint8_t>((bits + 7) / 8);
            }
        }
    }
    return 8;
}

} // namespace

void X86_64CodeEmitter::beginFunction(CodeEmitterContext *ctx, std::string_view name)
{
    m_ctx = ctx;
    if (!m_ctx)
    {
        return;
    }
    CodeSection *textSec = m_ctx->getSection(SectionType::Text);
    CodeLabel *entryLabel = m_ctx->getOrCreateLabel(textSec, MIRID_INVALID, name);
    m_ctx->bindLabel(entryLabel);
}

void X86_64CodeEmitter::beginFunction(CodeEmitterContext *ctx, MirFunction *func)
{
    m_currentFunc = func;
    beginFunction(ctx, func ? func->getName() : "");
}

void X86_64CodeEmitter::bindLabel(MirId labelId)
{
    if (!m_ctx)
    {
        return;
    }
    CodeSection *sec = m_ctx->getCurrentSection();
    CodeLabel *label = m_ctx->getOrCreateLabel(sec, labelId, "");
    m_ctx->bindLabel(label);
}

void X86_64CodeEmitter::endFunction(CodeEmitterContext *ctx) { endFunction(ctx, nullptr); }

void X86_64CodeEmitter::endFunction(CodeEmitterContext *ctx, MirFunction *func)
{
    if (ctx)
    {
        ctx->resetFuncState(func);
    }
    m_ctx = nullptr;
    m_currentFunc = nullptr;
}

uint8_t X86_64CodeEmitter::mapRegister(MirRegister *reg) const
{
    if (!reg)
    {
        return InvalidRegister;
    }

    size_t id = reg->getRegId();

    // FPR/XMM registers live in the >= 16 half of the unified encoding space.
    if (MirRegisterClass *regClass = reg->getRegClass())
    {
        std::string_view className = regClass->getName();
        if ((className.find("FPR") != std::string_view::npos ||
             className.find("VR") != std::string_view::npos) && id < 16)
        {
            return static_cast<uint8_t>(16 + id);
        }
    }

    // Integer registers pass through directly; anything beyond the 32-register space is invalid.
    return id < 32 ? static_cast<uint8_t>(id) : InvalidRegister;
}

bool X86_64CodeEmitter::buildResolvedOperands(const EncodingDesc &enc,
                                              std::span<MirOperand *const> operands,
                                              std::vector<ResolvedOperand> &resolved) const
{
    resolved.assign(operands.size(), ResolvedOperand{});

    // Returns true when the operand at index is bound to a memory slot in this encoding.
    auto usesMemorySlot = [&enc](size_t index)
    {
        for (uint8_t i = 0; i < enc.m_operandCount; ++i)
        {
            if (enc.m_operands[i].m_operandIndex == index && enc.m_operands[i].m_slot == EncSlotKind::RmMem)
            {
                return true;
            }
        }
        return false;
    };

    // Converts a MIR memory/reference operand into a target-neutral EncMemory form.
    auto buildMemory = [&](MirOperand *op)
    {
        EncMemory mem;
        if (op->getType() == MirOperandType::Memory)
        {
            auto *mirMem = op->get<MirMemory>();
            if (mirMem)
            {
                if (mirMem->hasBaseReg())
                {
                    uint8_t base = mapRegister(mirMem->getBase());
                    mem.m_base = isFprEncoding(base) ? fprIndex(base) : base;
                }
                if (mirMem->hasIndexReg())
                {
                    uint8_t index = mapRegister(mirMem->getIndex());
                    mem.m_index = isFprEncoding(index) ? fprIndex(index) : index;
                    mem.m_scale = mirMem->getScale();
                }
                if (mirMem->getDisplacement())
                {
                    mem.m_disp = mirMem->getDisplacement()->getValue().getI64();
                }
            }
        }
        else if (op->getType() == MirOperandType::Reference)
        {
            auto *ref = op->get<MirReference>();
            if (ref)
            {
                if (ref->isStackFrameObject())
                {
                    // Address stack slots as [rbp + (reference offset + allocated frame offset)].
                    int64_t offset = static_cast<int64_t>(ref->getOffset());
                    if (m_currentFunc && m_currentFunc->getStackFrame())
                    {
                        if (StackFrameObject *obj = m_currentFunc->getStackFrame()->getObjectFromId(ref->getRefId()))
                        {
                            offset += obj->m_offset;
                        }
                    }
                    mem.m_base = FramePointerEncoding;
                    mem.m_disp = offset;
                }
                else if (ref->isGlobalVar())
                {
                    // Globals are addressed RIP-relative, with the fixup recorded as a relocation.
                    mem.m_ripRel = true;
                    mem.m_needsReloc = true;
                    mem.m_disp = 0;
                }
            }
        }
        return mem;
    };

    for (size_t i = 0; i < operands.size(); ++i)
    {
        MirOperand *op = operands[i];
        if (!op)
        {
            continue;
        }

        ResolvedOperand &out = resolved[i];
        switch (op->getType())
        {
            case MirOperandType::Register:
            {
                auto *reg = op->get<MirRegister>();
                if (!reg)
                {
                    return false;
                }
                uint8_t physical = mapRegister(reg);
                out.m_kind = ResolvedOperand::Kind::Register;
                out.m_isFpr = isFprEncoding(physical);
                out.m_reg = isFprEncoding(physical) ? fprIndex(physical) : physical;
                out.m_sizeBytes = operandSizeBytes(op, reg);
                break;
            }
            case MirOperandType::Integer:
            {
                auto *imm = op->get<MirInteger>();
                out.m_kind = ResolvedOperand::Kind::Immediate;
                out.m_imm = imm ? imm->getValue().getI64() : 0;
                out.m_sizeBytes = operandSizeBytes(op, nullptr);
                break;
            }
            case MirOperandType::Memory:
            {
                out.m_kind = ResolvedOperand::Kind::Memory;
                out.m_mem = buildMemory(op);
                out.m_sizeBytes = operandSizeBytes(op, nullptr);
                break;
            }
            case MirOperandType::Reference:
            {
                if (usesMemorySlot(i))
                {
                    out.m_kind = ResolvedOperand::Kind::Memory;
                    out.m_mem = buildMemory(op);
                }
                else
                {
                    out.m_kind = ResolvedOperand::Kind::Immediate;
                    out.m_needsReloc = true;
                }
                break;
            }
            default:
                break;
        }
    }

    return true;
}

bool X86_64CodeEmitter::tryEmitTableDriven(const MirTargetInstructionDesc *desc, std::span<MirOperand *const> operands)
{
    if (!m_encodingResolver)
    {
        return false;
    }

    const EncodingDesc *enc = m_encodingResolver(desc);
    if (!enc || enc->m_form == EncForm::None)
    {
        return false;
    }

    CodeSection *sec = m_ctx->getCurrentSection();
    if (!sec)
    {
        return false;
    }

    std::vector<ResolvedOperand> resolved;
    if (!buildResolvedOperands(*enc, operands, resolved))
    {
        return false;
    }

    std::vector<uint8_t> bytes;

    // Two-address coalescing: materialize the copy implied by a destructive operation
    // whose destination and first source register differ.
    if (enc->m_coalesceSrc != 0xFF && enc->m_coalesceSrc < resolved.size() && !resolved.empty())
    {
        const ResolvedOperand &dst = resolved[0];
        const ResolvedOperand &src = resolved[enc->m_coalesceSrc];
        if (dst.m_kind == ResolvedOperand::Kind::Register && src.m_kind == ResolvedOperand::Kind::Register &&
            (dst.m_reg != src.m_reg || dst.m_isFpr != src.m_isFpr))
        {
            InstructionEncoder::encodeRegisterMove(dst, src, bytes);
        }
    }

    EzCodeEmitter::EncodeResult result;
    if (!InstructionEncoder::encode(*enc, resolved, bytes, result))
    {
        return false;
    }

    if (result.m_hasReloc)
    {
        // Locate the MIR reference operand that produced the relocation field.
        MirReference *relocRef = nullptr;
        for (size_t i = 0; i < resolved.size() && !relocRef; ++i)
        {
            bool needsReloc = resolved[i].m_needsReloc ||
                    (resolved[i].m_kind == ResolvedOperand::Kind::Memory && resolved[i].m_mem.m_needsReloc);
            if (needsReloc && operands[i]->getType() == MirOperandType::Reference)
            {
                relocRef = operands[i]->get<MirReference>();
            }
        }

        if (relocRef)
        {
            uint64_t start = sec->getCurrentOffset();
            if (result.m_isBranch)
            {
                // Branches carry their own PC-relative semantics.
                m_ctx->addReloc(relocRef, TargetCodeRelocationType::BranchRel32);
            }
            else
            {
                // Data references are fixed up at the exact relocation field offset.
                m_ctx->addRelocAt(relocRef, TargetCodeRelocationType::PCRel32, start + result.m_relocOffset);
            }
        }
    }

    if (!bytes.empty())
    {
        sec->emitBytes(bytes.data(), bytes.size());
    }
    return true;
}

void X86_64CodeEmitter::emitInst(const MirTargetInstructionDesc *desc, std::span<MirOperand *const> operands)
{
    if (!desc || !m_ctx)
    {
        return;
    }

    // All x86-64 instructions are emitted through the generated encoding table. A rejected
    // instruction previously vanished silently, producing a malformed object; surface it instead.
    if (!tryEmitTableDriven(desc, operands))
    {
        throw std::runtime_error(std::string("x86-64 emitter: no encoding available for instruction '") +
                                 std::string(desc->getName()) + "'");
    }
}

} // namespace EzTargets::X86_64
