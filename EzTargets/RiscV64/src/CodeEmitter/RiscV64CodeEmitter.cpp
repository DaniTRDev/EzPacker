#include "RiscV64CodeEmitter.h"
#include "CodeSection.h"
#include "CodeEmitterContext.h"
#include "Operand/MirOperands.h"
#include "Operand/MirRegisterClass.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionStackFrame.h"
#include "Type/MirType.h"
#include "Encoding/RiscV64InstructionEncoder.h"
#include <stdexcept>
#include <string>
#include <string_view>

namespace EzTargets::RiscV64
{

namespace
{

constexpr uint8_t InvalidRegister = 0xFF;
constexpr uint8_t FramePointerEncoding = 8; // s0

uint8_t operandSizeBytes(MirOperand *op, MirRegister *reg)
{
    if (op)
    {
        MirType *type = op->getMirType();
        if (type)
        {
            size_t bits = type->getTotalSizeInBits();
            if (bits > 0)
                return static_cast<uint8_t>((bits + 7) / 8);
        }
    }
    if (reg)
    {
        MirType *type = reg->getMirType();
        if (type)
        {
            size_t bits = type->getTotalSizeInBits();
            if (bits > 0)
                return static_cast<uint8_t>((bits + 7) / 8);
        }
    }
    return 8;
}

} // namespace

void RiscV64CodeEmitter::beginFunction(CodeEmitterContext *ctx, std::string_view name)
{
    m_ctx = ctx;
    if (!m_ctx)
        return;
    CodeSection *textSec = m_ctx->getSection(SectionType::Text);
    CodeLabel *entryLabel = m_ctx->getOrCreateLabel(textSec, MIRID_INVALID, name);
    m_ctx->bindLabel(entryLabel);
}

void RiscV64CodeEmitter::beginFunction(CodeEmitterContext *ctx, MirFunction *func)
{
    m_currentFunc = func;
    beginFunction(ctx, func ? func->getName() : "");
}

void RiscV64CodeEmitter::bindLabel(MirId labelId)
{
    if (!m_ctx)
        return;
    CodeSection *sec = m_ctx->getCurrentSection();
    CodeLabel *label = m_ctx->getOrCreateLabel(sec, labelId, "");
    m_ctx->bindLabel(label);
}

void RiscV64CodeEmitter::endFunction(CodeEmitterContext *ctx)
{
    endFunction(ctx, nullptr);
}

void RiscV64CodeEmitter::endFunction(CodeEmitterContext *ctx, MirFunction *func)
{
    if (ctx)
        ctx->resetFuncState(func);
    m_ctx = nullptr;
    m_currentFunc = nullptr;
}

uint8_t RiscV64CodeEmitter::mapRegister(MirRegister *reg, bool &isFpr) const
{
    if (!reg)
    {
        isFpr = false;
        return InvalidRegister;
    }

    size_t id = reg->getRegId();
    isFpr = false;
    if (MirRegisterClass *regClass = reg->getRegClass())
    {
        std::string_view className = regClass->getName();
        if (className.find("FPR") != std::string_view::npos)
        {
            isFpr = true;
        }
    }

    return id < 32 ? static_cast<uint8_t>(id) : InvalidRegister;
}

bool RiscV64CodeEmitter::buildResolvedOperands(const EncodingDesc &enc,
                                               std::span<MirOperand *const> operands,
                                               std::vector<ResolvedOperand> &resolved) const
{
    resolved.assign(operands.size(), ResolvedOperand{});

    auto usesMemorySlot = [&enc](size_t index)
    {
        for (uint8_t i = 0; i < enc.m_operandCount; ++i)
        {
            if (enc.m_operands[i].m_operandIndex == index &&
                (enc.m_operands[i].m_slot == EncSlotKind::Imm12Store ||
                 enc.m_operands[i].m_slot == EncSlotKind::Imm12Signed))
            {
                return true;
            }
        }
        return false;
    };

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
                    bool isFpr = false;
                    mem.m_base = mapRegister(mirMem->getBase(), isFpr);
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
                    int64_t offset = static_cast<int64_t>(ref->getOffset());
                    if (m_currentFunc && m_currentFunc->getStackFrame())
                    {
                        if (StackFrameObject *obj = m_currentFunc->getStackFrame()->getObjectFromId(ref->getRefId()))
                        {
                            offset += obj->m_offset;
                        }
                    }
                    mem.m_base = FramePointerEncoding; // s0
                    mem.m_disp = offset;
                }
                else if (ref->isGlobalVar() || ref->isFunction())
                {
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
            continue;

        ResolvedOperand &out = resolved[i];
        switch (op->getType())
        {
            case MirOperandType::Register:
            {
                auto *reg = op->get<MirRegister>();
                if (!reg)
                    return false;
                bool isFpr = false;
                uint8_t physical = mapRegister(reg, isFpr);
                out.m_kind = ResolvedOperand::Kind::Register;
                out.m_isFpr = isFpr;
                out.m_reg = physical;
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
                auto *ref = op->get<MirReference>();
                if (ref && ref->isStackFrameObject())
                {
                    int64_t offset = static_cast<int64_t>(ref->getOffset());
                    if (m_currentFunc && m_currentFunc->getStackFrame())
                    {
                        if (StackFrameObject *obj = m_currentFunc->getStackFrame()->getObjectFromId(ref->getRefId()))
                        {
                            offset += obj->m_offset;
                        }
                    }
                    out.m_kind = ResolvedOperand::Kind::Immediate;
                    out.m_imm = offset;
                }
                else if (usesMemorySlot(i))
                {
                    out.m_kind = ResolvedOperand::Kind::Memory;
                    out.m_mem = buildMemory(op);
                }
                else
                {
                    out.m_kind = ResolvedOperand::Kind::Immediate;
                    out.m_needsReloc = true;
                    out.m_imm = 0;
                }
                break;
            }
            default:
                break;
        }
    }

    return true;
}

bool RiscV64CodeEmitter::tryEmitTableDriven(const MirTargetInstructionDesc *desc, std::span<MirOperand *const> operands)
{
    if (!m_encodingResolver)
        return false;

    const EncodingDesc *enc = m_encodingResolver(desc);
    if (!enc || enc->m_form == EncForm::None)
        return false;

    CodeSection *sec = m_ctx->getCurrentSection();
    if (!sec)
        return false;

    std::vector<ResolvedOperand> resolved;
    if (!buildResolvedOperands(*enc, operands, resolved))
        return false;

    std::vector<uint8_t> bytes;
    EzCodeEmitter::EncodeResult result;
    if (!InstructionEncoder::encode(*enc, resolved, bytes, result))
        return false;

    if (result.m_hasReloc)
    {
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
                m_ctx->addReloc(relocRef, TargetCodeRelocationType::BranchRel32);
            }
            else
            {
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

void RiscV64CodeEmitter::emitInst(const MirTargetInstructionDesc *desc, std::span<MirOperand *const> operands)
{
    if (!desc || !m_ctx)
        return;

    if (!tryEmitTableDriven(desc, operands))
    {
        throw std::runtime_error(std::string("RISC-V 64 emitter: no encoding available for instruction '") +
                                 std::string(desc->getName()) + "'");
    }
}

} // namespace EzTargets::RiscV64
