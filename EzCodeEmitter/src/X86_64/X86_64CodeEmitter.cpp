#include "X86_64/X86_64CodeEmitter.h"
#include "CodeSection.h"
#include "CodeEmitterContext.h"
#include "Operand/MirOperands.h"
#include "Operand/MirRegisterClass.h"
#include <stdexcept>

namespace EzCodeEmitter::X86_64
{

X86_64CodeEmitter::X86_64CodeEmitter(RegMapper regMapper) :
    m_regMapper(std::move(regMapper))
{
}

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

void X86_64CodeEmitter::endFunction(CodeEmitterContext *ctx)
{
    endFunction(ctx, nullptr);
}

void X86_64CodeEmitter::endFunction(CodeEmitterContext *ctx, MirFunction *func)
{
    if (ctx)
    {
        ctx->resetFuncState(func);
    }
    m_ctx = nullptr;
}

Reg X86_64CodeEmitter::mapRegister(MirRegister *reg) const
{
    if (!reg)
    {
        return Reg::None;
    }
    size_t id = reg->getRegId();
    if (m_regMapper)
    {
        return m_regMapper(id);
    }
    if (reg->getRegClass())
    {
        std::string_view clsName = reg->getRegClass()->getName();
        if (clsName.find("FPR") != std::string_view::npos)
        {
            if (id < 16)
            {
                return static_cast<Reg>(16 + id);
            }
        }
    }
    if (id < 32)
    {
        return static_cast<Reg>(id);
    }
    return Reg::None;
}

MemoryOperand X86_64CodeEmitter::mapMemory(MirMemory *mem) const
{
    if (!mem)
    {
        return MemoryOperand::Base(Reg::None);
    }

    Reg baseReg = mem->hasBaseReg() ? mapRegister(mem->getBase()) : Reg::None;
    int64_t disp = 0;
    if (mem->getDisplacement())
    {
        disp = mem->getDisplacement()->getValue().getI64();
    }

    if (mem->hasIndexReg())
    {
        Reg indexReg = mapRegister(mem->getIndex());
        uint8_t scale = mem->getScale();
        if (baseReg != Reg::None)
        {
            return MemoryOperand::BaseIndex(baseReg, indexReg, scale, disp);
        }
        else
        {
            return MemoryOperand::IndexDisp(indexReg, scale, disp);
        }
    }

    return MemoryOperand::BaseDisp(baseReg, disp);
}

MemoryOperand X86_64CodeEmitter::mapOperandToMemory(MirOperand *op) const
{
    if (!op)
    {
        return MemoryOperand::Base(Reg::None);
    }
    if (op->getType() == MirOperandType::Memory)
    {
        return mapMemory(op->get<MirMemory>());
    }
    if (op->getType() == MirOperandType::Reference)
    {
        auto *ref = op->get<MirReference>();
        if (ref && ref->isStackFrameObject())
        {
            return MemoryOperand::BaseDisp(Reg::RBP, static_cast<int64_t>(ref->getOffset()));
        }
    }
    return MemoryOperand::Base(Reg::None);
}

void X86_64CodeEmitter::emitInst(MirTargetInstructionDesc *desc, std::span<MirOperand *> operands)
{
    if (!desc || !m_ctx)
    {
        return;
    }

    CodeSection *sec = m_ctx->getCurrentSection();
    if (!sec)
    {
        return;
    }

    std::string_view name = desc->getName();
    std::vector<uint8_t> bytes;

    auto commitBytes = [&]() {
        if (!bytes.empty())
        {
            sec->emitBytes(bytes.data(), bytes.size());
            bytes.clear();
        }
    };

    // Helper lambdas for repetitive patterns
    auto emitAluRRHelper = [&](AluOp op, uint8_t size) {
        if (operands.size() >= 3)
        {
            Reg dst = mapRegister(operands[0]->get<MirRegister>());
            Reg src1 = mapRegister(operands[1]->get<MirRegister>());
            Reg src2 = mapRegister(operands[2]->get<MirRegister>());
            if (dst != src1)
            {
                InstructionEncoder::emitMovRR(bytes, dst, src1, size);
            }
            InstructionEncoder::emitAluRR(bytes, op, dst, src2, size);
        }
        else if (operands.size() == 2)
        {
            Reg dst = mapRegister(operands[0]->get<MirRegister>());
            Reg src = mapRegister(operands[1]->get<MirRegister>());
            InstructionEncoder::emitAluRR(bytes, op, dst, src, size);
        }
        commitBytes();
    };

    auto emitAluRIHelper = [&](AluOp op, uint8_t size) {
        if (operands.size() >= 3)
        {
            Reg dst = mapRegister(operands[0]->get<MirRegister>());
            Reg src1 = mapRegister(operands[1]->get<MirRegister>());
            auto *imm = operands[2]->get<MirInteger>();
            int32_t val = imm ? imm->getValue().getI32() : 0;
            if (dst != src1)
            {
                InstructionEncoder::emitMovRR(bytes, dst, src1, size);
            }
            InstructionEncoder::emitAluRI(bytes, op, dst, val, size);
        }
        else if (operands.size() == 2)
        {
            Reg dst = mapRegister(operands[0]->get<MirRegister>());
            auto *imm = operands[1]->get<MirInteger>();
            int32_t val = imm ? imm->getValue().getI32() : 0;
            InstructionEncoder::emitAluRI(bytes, op, dst, val, size);
        }
        commitBytes();
    };

    auto emitAluRMHelper = [&](AluOp op, uint8_t size) {
        if (operands.size() >= 3)
        {
            Reg dst = mapRegister(operands[0]->get<MirRegister>());
            Reg src1 = mapRegister(operands[1]->get<MirRegister>());
            if (dst != src1)
            {
                InstructionEncoder::emitMovRR(bytes, dst, src1, size);
            }
            InstructionEncoder::emitAluRM(bytes, op, dst, mapOperandToMemory(operands[2]), size);
        }
        commitBytes();
    };

    auto emitShiftRIHelper = [&](void (*func)(std::vector<uint8_t> &, Reg, uint8_t, uint8_t), uint8_t size) {
        if (operands.size() >= 3)
        {
            Reg dst = mapRegister(operands[0]->get<MirRegister>());
            Reg src = mapRegister(operands[1]->get<MirRegister>());
            auto *imm = operands[2]->get<MirInteger>();
            uint8_t amt = imm ? static_cast<uint8_t>(imm->getValue().getU8()) : 0;
            if (dst != src)
            {
                InstructionEncoder::emitMovRR(bytes, dst, src, size);
            }
            func(bytes, dst, amt, size);
        }
        commitBytes();
    };

    auto emitShiftRCLHelper = [&](void (*func)(std::vector<uint8_t> &, Reg, uint8_t), uint8_t size) {
        if (operands.size() >= 2)
        {
            Reg dst = mapRegister(operands[0]->get<MirRegister>());
            Reg src = mapRegister(operands[1]->get<MirRegister>());
            if (dst != src)
            {
                InstructionEncoder::emitMovRR(bytes, dst, src, size);
            }
            func(bytes, dst, size);
        }
        commitBytes();
    };

    auto emitUnaryHelper = [&](void (*func)(std::vector<uint8_t> &, Reg, uint8_t), uint8_t size) {
        if (operands.size() >= 2)
        {
            Reg dst = mapRegister(operands[0]->get<MirRegister>());
            Reg src = mapRegister(operands[1]->get<MirRegister>());
            if (dst != src)
            {
                InstructionEncoder::emitMovRR(bytes, dst, src, size);
            }
            func(bytes, dst, size);
        }
        commitBytes();
    };

    auto emitJccHelper = [&](ConditionCode cc) {
        if (!operands.empty())
        {
            if (operands[0]->getType() == MirOperandType::Reference)
            {
                auto *ref = operands[0]->get<MirReference>();
                InstructionEncoder::emitJccNear(bytes, cc, 0);
                m_ctx->addReloc(ref, TargetCodeRelocationType::BranchRel32);
            }
            else if (operands[0]->getType() == MirOperandType::Integer)
            {
                int32_t disp = operands[0]->get<MirInteger>()->getValue().getI32();
                InstructionEncoder::emitJccNear(bytes, cc, disp);
            }
        }
        commitBytes();
    };

    auto emitSetccHelper = [&](ConditionCode cc) {
        if (!operands.empty())
        {
            auto *dst = operands[0]->get<MirRegister>();
            InstructionEncoder::emitSetcc(bytes, cc, mapRegister(dst));
        }
        commitBytes();
    };

    // =========================================================================
    // INTEGER ARITHMETIC
    // =========================================================================
    if (name == "ADD32rr") { emitAluRRHelper(AluOp::ADD, 4); return; }
    if (name == "ADD32ri") { emitAluRIHelper(AluOp::ADD, 4); return; }
    if (name == "ADD32rm") { emitAluRMHelper(AluOp::ADD, 4); return; }
    if (name == "ADD64rr") { emitAluRRHelper(AluOp::ADD, 8); return; }
    if (name == "ADD64ri") { emitAluRIHelper(AluOp::ADD, 8); return; }
    if (name == "ADD64rm") { emitAluRMHelper(AluOp::ADD, 8); return; }

    if (name == "SUB32rr") { emitAluRRHelper(AluOp::SUB, 4); return; }
    if (name == "SUB32ri") { emitAluRIHelper(AluOp::SUB, 4); return; }
    if (name == "SUB32rm") { emitAluRMHelper(AluOp::SUB, 4); return; }
    if (name == "SUB64rr") { emitAluRRHelper(AluOp::SUB, 8); return; }
    if (name == "SUB64ri") { emitAluRIHelper(AluOp::SUB, 8); return; }
    if (name == "SUB64rm") { emitAluRMHelper(AluOp::SUB, 8); return; }

    if (name == "IMUL32rr")
    {
        if (operands.size() >= 3)
        {
            Reg dst = mapRegister(operands[0]->get<MirRegister>());
            Reg src1 = mapRegister(operands[1]->get<MirRegister>());
            Reg src2 = mapRegister(operands[2]->get<MirRegister>());
            if (dst != src1) InstructionEncoder::emitMovRR(bytes, dst, src1, 4);
            InstructionEncoder::emitImulRR(bytes, dst, src2, 4);
        }
        commitBytes();
        return;
    }
    if (name == "IMUL32ri")
    {
        if (operands.size() >= 3)
        {
            Reg dst = mapRegister(operands[0]->get<MirRegister>());
            Reg src1 = mapRegister(operands[1]->get<MirRegister>());
            auto *imm = operands[2]->get<MirInteger>();
            int32_t val = imm ? imm->getValue().getI32() : 0;
            InstructionEncoder::emitImulRI(bytes, dst, src1, val, 4);
        }
        commitBytes();
        return;
    }
    if (name == "IMUL64rr")
    {
        if (operands.size() >= 3)
        {
            Reg dst = mapRegister(operands[0]->get<MirRegister>());
            Reg src1 = mapRegister(operands[1]->get<MirRegister>());
            Reg src2 = mapRegister(operands[2]->get<MirRegister>());
            if (dst != src1) InstructionEncoder::emitMovRR(bytes, dst, src1, 8);
            InstructionEncoder::emitImulRR(bytes, dst, src2, 8);
        }
        commitBytes();
        return;
    }
    if (name == "IMUL64ri")
    {
        if (operands.size() >= 3)
        {
            Reg dst = mapRegister(operands[0]->get<MirRegister>());
            Reg src1 = mapRegister(operands[1]->get<MirRegister>());
            auto *imm = operands[2]->get<MirInteger>();
            int32_t val = imm ? imm->getValue().getI32() : 0;
            InstructionEncoder::emitImulRI(bytes, dst, src1, val, 8);
        }
        commitBytes();
        return;
    }

    if (name == "IDIV32r")
    {
        if (!operands.empty()) InstructionEncoder::emitIdivR(bytes, mapRegister(operands[0]->get<MirRegister>()), 4);
        commitBytes();
        return;
    }
    if (name == "IDIV64r")
    {
        if (!operands.empty()) InstructionEncoder::emitIdivR(bytes, mapRegister(operands[0]->get<MirRegister>()), 8);
        commitBytes();
        return;
    }
    if (name == "DIV32r")
    {
        if (!operands.empty()) InstructionEncoder::emitDivR(bytes, mapRegister(operands[0]->get<MirRegister>()), 4);
        commitBytes();
        return;
    }
    if (name == "DIV64r")
    {
        if (!operands.empty()) InstructionEncoder::emitDivR(bytes, mapRegister(operands[0]->get<MirRegister>()), 8);
        commitBytes();
        return;
    }

    if (name == "NEG32r") { emitUnaryHelper(&InstructionEncoder::emitNegR, 4); return; }
    if (name == "NEG64r") { emitUnaryHelper(&InstructionEncoder::emitNegR, 8); return; }
    if (name == "NOT32r") { emitUnaryHelper(&InstructionEncoder::emitNotR, 4); return; }
    if (name == "NOT64r") { emitUnaryHelper(&InstructionEncoder::emitNotR, 8); return; }

    // =========================================================================
    // BITWISE & SHIFTS
    // =========================================================================
    if (name == "AND32rr") { emitAluRRHelper(AluOp::AND, 4); return; }
    if (name == "AND32ri") { emitAluRIHelper(AluOp::AND, 4); return; }
    if (name == "AND64rr") { emitAluRRHelper(AluOp::AND, 8); return; }
    if (name == "AND64ri") { emitAluRIHelper(AluOp::AND, 8); return; }

    if (name == "OR32rr") { emitAluRRHelper(AluOp::OR, 4); return; }
    if (name == "OR32ri") { emitAluRIHelper(AluOp::OR, 4); return; }
    if (name == "OR64rr") { emitAluRRHelper(AluOp::OR, 8); return; }
    if (name == "OR64ri") { emitAluRIHelper(AluOp::OR, 8); return; }

    if (name == "XOR32rr") { emitAluRRHelper(AluOp::XOR, 4); return; }
    if (name == "XOR32ri") { emitAluRIHelper(AluOp::XOR, 4); return; }
    if (name == "XOR64rr") { emitAluRRHelper(AluOp::XOR, 8); return; }
    if (name == "XOR64ri") { emitAluRIHelper(AluOp::XOR, 8); return; }

    if (name == "SHL32ri") { emitShiftRIHelper(&InstructionEncoder::emitShlRI, 4); return; }
    if (name == "SHL32rCL") { emitShiftRCLHelper(&InstructionEncoder::emitShlRCL, 4); return; }
    if (name == "SHL64ri") { emitShiftRIHelper(&InstructionEncoder::emitShlRI, 8); return; }
    if (name == "SHL64rCL") { emitShiftRCLHelper(&InstructionEncoder::emitShlRCL, 8); return; }

    if (name == "SHR32ri") { emitShiftRIHelper(&InstructionEncoder::emitShrRI, 4); return; }
    if (name == "SHR32rCL") { emitShiftRCLHelper(&InstructionEncoder::emitShrRCL, 4); return; }
    if (name == "SHR64ri") { emitShiftRIHelper(&InstructionEncoder::emitShrRI, 8); return; }
    if (name == "SHR64rCL") { emitShiftRCLHelper(&InstructionEncoder::emitShrRCL, 8); return; }

    if (name == "SAR32ri") { emitShiftRIHelper(&InstructionEncoder::emitSarRI, 4); return; }
    if (name == "SAR32rCL") { emitShiftRCLHelper(&InstructionEncoder::emitSarRCL, 4); return; }
    if (name == "SAR64ri") { emitShiftRIHelper(&InstructionEncoder::emitSarRI, 8); return; }
    if (name == "SAR64rCL") { emitShiftRCLHelper(&InstructionEncoder::emitSarRCL, 8); return; }

    // =========================================================================
    // COMPARISONS & SETcc
    // =========================================================================
    if (name == "CMP32rr")
    {
        if (operands.size() >= 2)
        {
            InstructionEncoder::emitAluRR(bytes, AluOp::CMP, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()), 4);
        }
        commitBytes();
        return;
    }
    if (name == "CMP32ri")
    {
        if (operands.size() >= 2)
        {
            auto *imm = operands[1]->get<MirInteger>();
            int32_t val = imm ? imm->getValue().getI32() : 0;
            InstructionEncoder::emitAluRI(bytes, AluOp::CMP, mapRegister(operands[0]->get<MirRegister>()), val, 4);
        }
        commitBytes();
        return;
    }
    if (name == "CMP64rr")
    {
        if (operands.size() >= 2)
        {
            InstructionEncoder::emitAluRR(bytes, AluOp::CMP, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()), 8);
        }
        commitBytes();
        return;
    }
    if (name == "CMP64ri")
    {
        if (operands.size() >= 2)
        {
            auto *imm = operands[1]->get<MirInteger>();
            int32_t val = imm ? imm->getValue().getI32() : 0;
            InstructionEncoder::emitAluRI(bytes, AluOp::CMP, mapRegister(operands[0]->get<MirRegister>()), val, 8);
        }
        commitBytes();
        return;
    }

    if (name == "TEST32rr")
    {
        if (operands.size() >= 2)
        {
            InstructionEncoder::emitTestRR(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()), 4);
        }
        commitBytes();
        return;
    }
    if (name == "TEST64rr")
    {
        if (operands.size() >= 2)
        {
            InstructionEncoder::emitTestRR(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()), 8);
        }
        commitBytes();
        return;
    }

    if (name == "SETE")  { emitSetccHelper(ConditionCode::E);  return; }
    if (name == "SETNE") { emitSetccHelper(ConditionCode::NE); return; }
    if (name == "SETL")  { emitSetccHelper(ConditionCode::L);  return; }
    if (name == "SETLE") { emitSetccHelper(ConditionCode::LE); return; }
    if (name == "SETG")  { emitSetccHelper(ConditionCode::G);  return; }
    if (name == "SETGE") { emitSetccHelper(ConditionCode::GE); return; }
    if (name == "SETB")  { emitSetccHelper(ConditionCode::B);  return; }
    if (name == "SETBE") { emitSetccHelper(ConditionCode::BE); return; }
    if (name == "SETA")  { emitSetccHelper(ConditionCode::A);  return; }
    if (name == "SETAE") { emitSetccHelper(ConditionCode::AE); return; }

    // =========================================================================
    // DATA MOVEMENT & MEMORY
    // =========================================================================
    if (name == "MOV8rr")
    {
        if (operands.size() >= 2) InstructionEncoder::emitMovRR(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()), 1);
        commitBytes();
        return;
    }
    if (name == "MOV8ri")
    {
        if (operands.size() >= 2)
        {
            auto *imm = operands[1]->get<MirInteger>();
            int64_t val = imm ? imm->getValue().getI64() : 0;
            InstructionEncoder::emitMovRI(bytes, mapRegister(operands[0]->get<MirRegister>()), val, 1);
        }
        commitBytes();
        return;
    }
    if (name == "MOV16rr")
    {
        if (operands.size() >= 2) InstructionEncoder::emitMovRR(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()), 2);
        commitBytes();
        return;
    }
    if (name == "MOV16ri")
    {
        if (operands.size() >= 2)
        {
            auto *imm = operands[1]->get<MirInteger>();
            int64_t val = imm ? imm->getValue().getI64() : 0;
            InstructionEncoder::emitMovRI(bytes, mapRegister(operands[0]->get<MirRegister>()), val, 2);
        }
        commitBytes();
        return;
    }
    if (name == "MOV32rr")
    {
        if (operands.size() >= 2) InstructionEncoder::emitMovRR(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()), 4);
        commitBytes();
        return;
    }
    if (name == "MOV32ri")
    {
        if (operands.size() >= 2)
        {
            auto *imm = operands[1]->get<MirInteger>();
            int64_t val = imm ? imm->getValue().getI64() : 0;
            InstructionEncoder::emitMovRI(bytes, mapRegister(operands[0]->get<MirRegister>()), val, 4);
        }
        commitBytes();
        return;
    }
    if (name == "MOV64rr")
    {
        if (operands.size() >= 2) InstructionEncoder::emitMovRR(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()), 8);
        commitBytes();
        return;
    }
    if (name == "MOV64ri")
    {
        if (operands.size() >= 2)
        {
            auto *imm = operands[1]->get<MirInteger>();
            int64_t val = imm ? imm->getValue().getI64() : 0;
            InstructionEncoder::emitMovRI(bytes, mapRegister(operands[0]->get<MirRegister>()), val, 8);
        }
        commitBytes();
        return;
    }

    if (name == "LOAD8")
    {
        if (operands.size() >= 2) InstructionEncoder::emitMovRM(bytes, mapRegister(operands[0]->get<MirRegister>()), mapOperandToMemory(operands[1]), 1);
        commitBytes();
        return;
    }
    if (name == "LOAD16")
    {
        if (operands.size() >= 2) InstructionEncoder::emitMovRM(bytes, mapRegister(operands[0]->get<MirRegister>()), mapOperandToMemory(operands[1]), 2);
        commitBytes();
        return;
    }
    if (name == "LOAD32")
    {
        if (operands.size() >= 2) InstructionEncoder::emitMovRM(bytes, mapRegister(operands[0]->get<MirRegister>()), mapOperandToMemory(operands[1]), 4);
        commitBytes();
        return;
    }
    if (name == "LOAD64")
    {
        if (operands.size() >= 2) InstructionEncoder::emitMovRM(bytes, mapRegister(operands[0]->get<MirRegister>()), mapOperandToMemory(operands[1]), 8);
        commitBytes();
        return;
    }

    if (name == "STORE8")
    {
        if (operands.size() >= 2) InstructionEncoder::emitMovMR(bytes, mapOperandToMemory(operands[0]), mapRegister(operands[1]->get<MirRegister>()), 1);
        commitBytes();
        return;
    }
    if (name == "STORE16")
    {
        if (operands.size() >= 2) InstructionEncoder::emitMovMR(bytes, mapOperandToMemory(operands[0]), mapRegister(operands[1]->get<MirRegister>()), 2);
        commitBytes();
        return;
    }
    if (name == "STORE32")
    {
        if (operands.size() >= 2) InstructionEncoder::emitMovMR(bytes, mapOperandToMemory(operands[0]), mapRegister(operands[1]->get<MirRegister>()), 4);
        commitBytes();
        return;
    }
    if (name == "STORE64")
    {
        if (operands.size() >= 2) InstructionEncoder::emitMovMR(bytes, mapOperandToMemory(operands[0]), mapRegister(operands[1]->get<MirRegister>()), 8);
        commitBytes();
        return;
    }

    if (name == "MOVSX32_8")  { if (operands.size() >= 2) InstructionEncoder::emitMovsxRR(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()), 1); commitBytes(); return; }
    if (name == "MOVSX32_16") { if (operands.size() >= 2) InstructionEncoder::emitMovsxRR(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()), 2); commitBytes(); return; }
    if (name == "MOVSX64_8")  { if (operands.size() >= 2) InstructionEncoder::emitMovsxRR(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()), 1); commitBytes(); return; }
    if (name == "MOVSX64_16") { if (operands.size() >= 2) InstructionEncoder::emitMovsxRR(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()), 2); commitBytes(); return; }
    if (name == "MOVSX64_32") { if (operands.size() >= 2) InstructionEncoder::emitMovsxRR(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()), 4); commitBytes(); return; }

    if (name == "MOVZX32_8")  { if (operands.size() >= 2) InstructionEncoder::emitMovzxRR(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()), 1); commitBytes(); return; }
    if (name == "MOVZX32_16") { if (operands.size() >= 2) InstructionEncoder::emitMovzxRR(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()), 2); commitBytes(); return; }
    if (name == "MOVZX64_8")  { if (operands.size() >= 2) InstructionEncoder::emitMovzxRR(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()), 1); commitBytes(); return; }
    if (name == "MOVZX64_16") { if (operands.size() >= 2) InstructionEncoder::emitMovzxRR(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()), 2); commitBytes(); return; }

    if (name == "LEA64" || name == "LEA64r")
    {
        if (operands.size() >= 2)
        {
            auto *dst = operands[0]->get<MirRegister>();
            InstructionEncoder::emitLea(bytes, mapRegister(dst), mapOperandToMemory(operands[1]), 8);
        }
        commitBytes();
        return;
    }

    // =========================================================================
    // CONTROL FLOW
    // =========================================================================
    if (name == "JMP")
    {
        if (!operands.empty())
        {
            if (operands[0]->getType() == MirOperandType::Reference)
            {
                auto *ref = operands[0]->get<MirReference>();
                InstructionEncoder::emitJmpNear(bytes, 0);
                m_ctx->addReloc(ref, TargetCodeRelocationType::BranchRel32);
            }
            else if (operands[0]->getType() == MirOperandType::Integer)
            {
                int32_t disp = operands[0]->get<MirInteger>()->getValue().getI32();
                InstructionEncoder::emitJmpNear(bytes, disp);
            }
            else if (operands[0]->getType() == MirOperandType::Register)
            {
                auto *reg = operands[0]->get<MirRegister>();
                InstructionEncoder::emitJmpR(bytes, mapRegister(reg));
            }
        }
        commitBytes();
        return;
    }

    if (name == "JE")  { emitJccHelper(ConditionCode::E);  return; }
    if (name == "JNE") { emitJccHelper(ConditionCode::NE); return; }
    if (name == "JL")  { emitJccHelper(ConditionCode::L);  return; }
    if (name == "JLE") { emitJccHelper(ConditionCode::LE); return; }
    if (name == "JG")  { emitJccHelper(ConditionCode::G);  return; }
    if (name == "JGE") { emitJccHelper(ConditionCode::GE); return; }
    if (name == "JB")  { emitJccHelper(ConditionCode::B);  return; }
    if (name == "JBE") { emitJccHelper(ConditionCode::BE); return; }
    if (name == "JA")  { emitJccHelper(ConditionCode::A);  return; }
    if (name == "JAE") { emitJccHelper(ConditionCode::AE); return; }

    if (name == "CALL")
    {
        if (!operands.empty())
        {
            if (operands[0]->getType() == MirOperandType::Register)
            {
                auto *reg = operands[0]->get<MirRegister>();
                InstructionEncoder::emitCallR(bytes, mapRegister(reg));
            }
            else if (operands[0]->getType() == MirOperandType::Reference)
            {
                auto *ref = operands[0]->get<MirReference>();
                InstructionEncoder::emitCallNear(bytes, 0);
                m_ctx->addReloc(ref, TargetCodeRelocationType::BranchRel32);
            }
            else if (operands[0]->getType() == MirOperandType::Integer)
            {
                int32_t disp = operands[0]->get<MirInteger>()->getValue().getI32();
                InstructionEncoder::emitCallNear(bytes, disp);
            }
        }
        commitBytes();
        return;
    }

    if (name == "RET")
    {
        InstructionEncoder::emitRet(bytes);
        commitBytes();
        return;
    }

    if (name == "PUSH64r")
    {
        if (!operands.empty())
        {
            auto *reg = operands[0]->get<MirRegister>();
            InstructionEncoder::emitPushR(bytes, mapRegister(reg));
        }
        commitBytes();
        return;
    }

    if (name == "POP64r")
    {
        if (!operands.empty())
        {
            auto *reg = operands[0]->get<MirRegister>();
            InstructionEncoder::emitPopR(bytes, mapRegister(reg));
        }
        commitBytes();
        return;
    }

    // =========================================================================
    // FLOATING POINT (SSE)
    // =========================================================================
    if (name == "ADDSS")
    {
        if (operands.size() >= 3)
        {
            Reg dst = mapRegister(operands[0]->get<MirRegister>());
            Reg src1 = mapRegister(operands[1]->get<MirRegister>());
            Reg src2 = mapRegister(operands[2]->get<MirRegister>());
            if (dst != src1) InstructionEncoder::emitMovssRR(bytes, dst, src1);
            InstructionEncoder::emitAddss(bytes, dst, src2);
        }
        commitBytes();
        return;
    }
    if (name == "ADDSD")
    {
        if (operands.size() >= 3)
        {
            Reg dst = mapRegister(operands[0]->get<MirRegister>());
            Reg src1 = mapRegister(operands[1]->get<MirRegister>());
            Reg src2 = mapRegister(operands[2]->get<MirRegister>());
            if (dst != src1) InstructionEncoder::emitMovsdRR(bytes, dst, src1);
            InstructionEncoder::emitAddsd(bytes, dst, src2);
        }
        commitBytes();
        return;
    }
    if (name == "SUBSS")
    {
        if (operands.size() >= 3)
        {
            Reg dst = mapRegister(operands[0]->get<MirRegister>());
            Reg src1 = mapRegister(operands[1]->get<MirRegister>());
            Reg src2 = mapRegister(operands[2]->get<MirRegister>());
            if (dst != src1) InstructionEncoder::emitMovssRR(bytes, dst, src1);
            InstructionEncoder::emitSubss(bytes, dst, src2);
        }
        commitBytes();
        return;
    }
    if (name == "SUBSD")
    {
        if (operands.size() >= 3)
        {
            Reg dst = mapRegister(operands[0]->get<MirRegister>());
            Reg src1 = mapRegister(operands[1]->get<MirRegister>());
            Reg src2 = mapRegister(operands[2]->get<MirRegister>());
            if (dst != src1) InstructionEncoder::emitMovsdRR(bytes, dst, src1);
            InstructionEncoder::emitSubsd(bytes, dst, src2);
        }
        commitBytes();
        return;
    }
    if (name == "MULSS")
    {
        if (operands.size() >= 3)
        {
            Reg dst = mapRegister(operands[0]->get<MirRegister>());
            Reg src1 = mapRegister(operands[1]->get<MirRegister>());
            Reg src2 = mapRegister(operands[2]->get<MirRegister>());
            if (dst != src1) InstructionEncoder::emitMovssRR(bytes, dst, src1);
            InstructionEncoder::emitMulss(bytes, dst, src2);
        }
        commitBytes();
        return;
    }
    if (name == "MULSD")
    {
        if (operands.size() >= 3)
        {
            Reg dst = mapRegister(operands[0]->get<MirRegister>());
            Reg src1 = mapRegister(operands[1]->get<MirRegister>());
            Reg src2 = mapRegister(operands[2]->get<MirRegister>());
            if (dst != src1) InstructionEncoder::emitMovsdRR(bytes, dst, src1);
            InstructionEncoder::emitMulsd(bytes, dst, src2);
        }
        commitBytes();
        return;
    }
    if (name == "DIVSS")
    {
        if (operands.size() >= 3)
        {
            Reg dst = mapRegister(operands[0]->get<MirRegister>());
            Reg src1 = mapRegister(operands[1]->get<MirRegister>());
            Reg src2 = mapRegister(operands[2]->get<MirRegister>());
            if (dst != src1) InstructionEncoder::emitMovssRR(bytes, dst, src1);
            InstructionEncoder::emitDivss(bytes, dst, src2);
        }
        commitBytes();
        return;
    }
    if (name == "DIVSD")
    {
        if (operands.size() >= 3)
        {
            Reg dst = mapRegister(operands[0]->get<MirRegister>());
            Reg src1 = mapRegister(operands[1]->get<MirRegister>());
            Reg src2 = mapRegister(operands[2]->get<MirRegister>());
            if (dst != src1) InstructionEncoder::emitMovsdRR(bytes, dst, src1);
            InstructionEncoder::emitDivsd(bytes, dst, src2);
        }
        commitBytes();
        return;
    }

    if (name == "MOVSSrr")
    {
        if (operands.size() >= 2) InstructionEncoder::emitMovssRR(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()));
        commitBytes();
        return;
    }
    if (name == "MOVSDrr")
    {
        if (operands.size() >= 2) InstructionEncoder::emitMovsdRR(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()));
        commitBytes();
        return;
    }

    if (name == "UCOMISS")
    {
        if (operands.size() >= 2) InstructionEncoder::emitUcomiss(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()));
        commitBytes();
        return;
    }
    if (name == "UCOMISD")
    {
        if (operands.size() >= 2) InstructionEncoder::emitUcomisd(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()));
        commitBytes();
        return;
    }

    if (name == "CVTSI2SS")
    {
        if (operands.size() >= 2) InstructionEncoder::emitCvtsi2ss(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()), 4);
        commitBytes();
        return;
    }
    if (name == "CVTSI2SD")
    {
        if (operands.size() >= 2) InstructionEncoder::emitCvtsi2sd(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()), 8);
        commitBytes();
        return;
    }
    if (name == "CVTTSS2SI")
    {
        if (operands.size() >= 2) InstructionEncoder::emitCvttss2si(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()), 4);
        commitBytes();
        return;
    }
    if (name == "CVTTSD2SI")
    {
        if (operands.size() >= 2) InstructionEncoder::emitCvttsd2si(bytes, mapRegister(operands[0]->get<MirRegister>()), mapRegister(operands[1]->get<MirRegister>()), 8);
        commitBytes();
        return;
    }

    // =========================================================================
    // SYSTEM
    // =========================================================================
    if (name == "SYSCALL")
    {
        InstructionEncoder::emitSyscall(bytes);
        commitBytes();
        return;
    }

    if (name == "NOP")
    {
        InstructionEncoder::emitNop(bytes, 1);
        commitBytes();
        return;
    }
}

} // namespace EzCodeEmitter::X86_64
