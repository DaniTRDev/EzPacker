#include "CodeEmitter/EzTestTripleCodeEmitter.h"

void EzTestTripleCodeEmitter::beginFunction(CodeEmitterContext *ctx, std::string_view name)
{
    m_ctx = ctx;

    // Ensure we start emitting in the default .text section
    CodeSection *textSec = m_ctx->getSection(SectionType::Text);
    if (textSec != nullptr)
    {
        // Align function entry point (16-byte alignment with padding)
        textSec->alignTo(textSec->getAlignment());
    }
}

void EzTestTripleCodeEmitter::bindLabel(MirId labelId)
{
    if (!m_ctx)
    {
        return;
    }

    // Default label placement occurs in the .text section
    CodeSection *textSec = m_ctx->getSection(SectionType::Text);
    CodeLabel *label = m_ctx->getOrCreateLabel(textSec, labelId, "");

    // Bind the label in context (attaches/rewinds SectionNode in CodeSection)
    m_ctx->bindLabel(label);
}

void EzTestTripleCodeEmitter::endFunction(CodeEmitterContext *ctx) { m_ctx = ctx; }

void EzTestTripleCodeEmitter::emitInst(MirTargetInstructionDesc *desc, std::span<MirOperand *> operands)
{
    if (!m_ctx || !desc)
    {
        return;
    }

    CodeSection *sec = m_ctx->getCurrentSection();
    if (!sec)
    {
        return;
    }

    const uint8_t targetId = static_cast<uint8_t>(desc->getId() & 0xFF);
    uint8_t op0 = 0;
    uint8_t op1 = 0;
    uint8_t aux = 0;
    int32_t memDisplacement = 0;

    // =========================================================================
    // 1. Identify RIP-Relative Instructions by ID Range
    // =========================================================================
    const bool isRipRelative = (targetId == 13) || // LEA64rm_rip
            (targetId >= 20 && targetId <= 25) ||  // MOV*rm_rip
            (targetId >= 36 && targetId <= 41);    // MOV*mr_rip

    if (isRipRelative)
    {
        aux = 0x01; // RIP-relative addressing mode
    }

    // =========================================================================
    // 2. Decode Operands and Extract Register Indices / Memory Addressing
    // =========================================================================
    if (!operands.empty() && operands[0] != nullptr)
    {
        if (const auto *reg = operands[0]->get<MirRegister>())
        {
            op0 = static_cast<uint8_t>(reg->getRegId() & 0xFF);
        }
        else if (const auto *mem = operands[0]->get<MirMemory>())
        {
            if (!isRipRelative)
            {
                aux = 0x02; // Base + displacement
            }
            if (mem->hasBaseReg())
            {
                op0 = static_cast<uint8_t>(mem->getBase()->getRegId() & 0xFF);
            }
            if (mem->getDisplacement())
            {
                memDisplacement = static_cast<int32_t>(mem->getDisplacement()->getValue().getI64());
            }
        }
    }

    if (operands.size() >= 2 && operands[1] != nullptr)
    {
        if (const auto *reg = operands[1]->get<MirRegister>())
        {
            op1 = static_cast<uint8_t>(reg->getRegId() & 0xFF);
        }
        else if (const auto *mem = operands[1]->get<MirMemory>())
        {
            if (!isRipRelative)
            {
                aux = 0x02; // Base + displacement
            }
            if (mem->hasBaseReg())
            {
                op1 = static_cast<uint8_t>(mem->getBase()->getRegId() & 0xFF);
            }
            if (mem->getDisplacement())
            {
                memDisplacement = static_cast<int32_t>(mem->getDisplacement()->getValue().getI64());
            }
        }
    }

    // =========================================================================
    // 3. Emit 4-Byte Base Header [OpcodeID, Op0, Op1, Aux]
    // =========================================================================
    sec->emit8(targetId);
    sec->emit8(op0);
    sec->emit8(op1);
    sec->emit8(aux);

    // =========================================================================
    // 4. Emit Memory Displacement (if Aux == 0x02)
    // =========================================================================
    if (aux == 0x02)
    {
        sec->emit32(static_cast<uint32_t>(memDisplacement));
        return;
    }

    // =========================================================================
    // 5. Emit Immediates, Relocations, and Branch Target Payloads
    // =========================================================================
    for (MirOperand *op : operands)
    {
        if (!op)
        {
            continue;
        }

        // --- Integer Immediates ---
        if (const auto *immInt = op->get<MirInteger>())
        {
            const int64_t val = immInt->getValue().getI64();
            if (targetId == 11 /* MOVABS64ri */)
            {
                sec->emit64(static_cast<uint64_t>(val));
            }
            else
            {
                sec->emit32(static_cast<uint32_t>(val & 0xFFFFFFFF));
            }
            return;
        }

        // --- Floating Point Immediates ---
        if (const auto *immFloat = op->get<MirFloat>())
        {
            if (op->getSizeInBytes() == 8)
            {
                double dVal = immFloat->getValue().getDouble();
                sec->emit64(*reinterpret_cast<uint64_t *>(&dVal));
            }
            else
            {
                float fVal = immFloat->getValue().getFloat();
                sec->emit32(*reinterpret_cast<uint32_t *>(&fVal));
            }
            return;
        }

        // --- Symbolic References (Functions, Globals, Blocks) ---
        if (auto *ref = op->get<MirReference>())
        {
            if (ref->isBlock())
            {
                // JMP, JE, JNE, etc. (Target IDs 204 to 212)
                m_ctx->addReloc(ref, TargetCodeRelocationType::BranchRel32);
                sec->emit32(0);
            }
            else if (ref->isFunction())
            {
                // Direct CALL (Target ID 215)
                m_ctx->addReloc(ref, TargetCodeRelocationType::PLTRel32);
                sec->emit32(0);
            }
            else if (targetId == 11 /* MOVABS64ri */)
            {
                m_ctx->addReloc(ref, TargetCodeRelocationType::Absolute64);
                sec->emit64(0);
            }
            else if (isRipRelative)
            {
                m_ctx->addReloc(ref, TargetCodeRelocationType::PCRel32);
                sec->emit32(0);
            }
            else
            {
                m_ctx->addReloc(ref, TargetCodeRelocationType::Absolute32);
                sec->emit32(0);
            }
            return;
        }

        // --- Runtime ABI Symbols (@memcpy, @__udivti3) ---
        if (auto *rtSym = op->get<MirRuntimeSymbol>())
        {
            m_ctx->addReloc(rtSym->get<MirReference>(), TargetCodeRelocationType::PLTRel32);
            sec->emit32(0);
            return;
        }
    }
}