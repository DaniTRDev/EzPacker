#include "EzTripleTestLegalizer.h"

void EzTripleTestLegalizer::create(MirBuilderContext *ctx, MirLegalizer *legalizer)
{
    const auto &t = ctx->getTypeTable();

    // Cache legal primitive target types for x86-64
    MirType *i8 = t->i8();
    MirType *i16 = t->i16();
    MirType *i32 = t->i32();
    MirType *i64 = t->i64();
    MirType *f32 = t->f32();
    MirType *f64 = t->f64();

    std::initializer_list<MirType *> legalTypes = { i8, i16, i32, i64, f32, f64 };
    std::initializer_list<MirType *> legalInts = { i8, i16, i32, i64 };

    // Instantiate the stateful rule builder
    LegalizeRuleBuilder builder(ctx, legalizer);

    // =========================================================================
    // 1. DATA MOVEMENT & MEMORY ACCESS
    // =========================================================================
    builder.begin(MirInstructionOpCode::MOV)
            .minSize(0, i8) // Intercept small types first!
            .legalForDest(legalTypes)
            .dump();

    builder.begin(MirInstructionOpCode::LOAD)
            .expandIf([](const LegalizeRuleOperand &op)
                      { return op.m_instr->getOperands()[0]->getMirType()->getTotalSizeInBits() > 64; })
            .minSize(0, i8)
            .legalForDest(legalTypes)
            .dump();

    builder.begin(MirInstructionOpCode::STORE)
            .expandIf([](const LegalizeRuleOperand &op)
                      { return op.m_instr->getOperands()[1]->getMirType()->getTotalSizeInBits() > 64; })
            .minSize(1, i8)
            .legalForSrc(legalTypes)
            .dump();

    builder.begin(MirInstructionOpCode::ALLOC)
            .legalIf([](const LegalizeRuleOperand &op)
                     { return op.m_instr->getOperands()[0]->getMirType()->getKind() == MirTypeKind::Pointer; })
            .dump();

    // =========================================================================
    // 2. ARITHMETIC & LOGIC (ALU)
    // =========================================================================
    std::initializer_list<MirInstructionOpCode> standardAluOps = {
        MirInstructionOpCode::ADD,  MirInstructionOpCode::SUB, MirInstructionOpCode::AND,
        MirInstructionOpCode::OR,   MirInstructionOpCode::XOR, MirInstructionOpCode::CMP,
        MirInstructionOpCode::TEST, MirInstructionOpCode::ADC, MirInstructionOpCode::SBB
    };

    for (auto opCode : standardAluOps)
    {
        builder.begin(opCode)
                .expandIf(
                        [](const LegalizeRuleOperand &op)
                        {
                            // 1. Force scalar expansion if any operand exceeds native 64-bit bounds
                            for (auto *operand : op.m_instr->getOperands())
                            {
                                if (operand->getMirType()->getTotalSizeInBits() > 64)
                                    return true;
                            }
                            return false;
                        })
                .minSize(0, i8) // 2. Catch and promote if dest is under-sized
                .minSize(1, i8) // 3. FIXED: Intercept and promote if source (operand 1) is under-sized (e.g., i1)
                .legalForDest(legalInts) // 4. Only accept native sizes if everything else passes
                .dump();
    }

    std::initializer_list<MirInstructionOpCode> wideAluOps = { MirInstructionOpCode::MUL,
                                                               MirInstructionOpCode::IMUL,
                                                               MirInstructionOpCode::DIV,
                                                               MirInstructionOpCode::IDIV,
                                                               MirInstructionOpCode::REM };

    for (auto opCode : wideAluOps)
    {
        builder.begin(opCode)
                .expandIf(
                        [](const LegalizeRuleOperand &op)
                        {
                            for (auto *operand : op.m_instr->getOperands())
                            {
                                if (operand->getMirType()->getTotalSizeInBits() > 64)
                                    return true;
                            }
                            return false;
                        })
                .minSize(0, i16)
                .minSize(1, i16)
                .legalForDest({ i16, i32, i64 })
                .dump();
    }

    builder.begin(MirInstructionOpCode::NEG)
            .expandIf([](const LegalizeRuleOperand &op)
                      { return op.m_instr->getOperands()[0]->getMirType()->getTotalSizeInBits() > 64; })
            .minSize(0, i8)
            .legalForDest(legalInts)
            .dump();

    builder.begin(MirInstructionOpCode::NOT)
            .expandIf([](const LegalizeRuleOperand &op)
                      { return op.m_instr->getOperands()[0]->getMirType()->getTotalSizeInBits() > 64; })
            .minSize(0, i8)
            .legalForDest(legalInts)
            .dump();

    std::initializer_list<MirInstructionOpCode> shiftOps = { MirInstructionOpCode::SHL,
                                                             MirInstructionOpCode::SHR,
                                                             MirInstructionOpCode::SAR };
    for (auto opCode : shiftOps)
    {
        builder.begin(opCode).minSize(0, i8).minSize(1, i8).legalForDest(legalInts).dump();
    }

    // =========================================================================
    // 3. CASTING & EXTENSIONS
    // =========================================================================
    builder.begin(MirInstructionOpCode::ZEXT).legalForDest(legalInts).dump();
    builder.begin(MirInstructionOpCode::SEXT).legalForDest(legalInts).dump();
    builder.begin(MirInstructionOpCode::TRUNC).legalForDest(legalInts).dump();
    builder.begin(MirInstructionOpCode::FPEXT).legalForDest({ f64 }).dump();
    builder.begin(MirInstructionOpCode::BITCAST).legalForDest(legalTypes).dump();

    // =========================================================================
    // 4. CONTROL FLOW & SYSTEM RULES
    // =========================================================================
    std::initializer_list<MirInstructionOpCode> branches = { MirInstructionOpCode::JMP, MirInstructionOpCode::JE,
                                                             MirInstructionOpCode::JNE, MirInstructionOpCode::JG,
                                                             MirInstructionOpCode::JGE, MirInstructionOpCode::JL,
                                                             MirInstructionOpCode::JLE, MirInstructionOpCode::JA,
                                                             MirInstructionOpCode::JB };
    for (auto opCode : branches)
    {
        builder.begin(opCode).legalIf([](const LegalizeRuleOperand &) { return true; }).dump();
    }

    size_t tokenTypeId = t->getBindingToken()->getId();

    builder.begin(MirInstructionOpCode::PUSH_ARG)
            .minSize(1, i8)
            .expandIf([](const LegalizeRuleOperand &op)
                      { return op.m_instr->getOperands()[1]->getMirType()->getTotalSizeInBits() > 64; })
            .legalIf([tokenTypeId](const LegalizeRuleOperand &op)
                     { return op.m_instr->getOperands()[0]->getMirType()->getId() == tokenTypeId; })
            .dump();

    builder.begin(MirInstructionOpCode::PUSH_RET)
            .minSize(1, i8)
            .expandIf([](const LegalizeRuleOperand &op)
                      { return op.m_instr->getOperands()[1]->getMirType()->getTotalSizeInBits() > 64; })
            .legalIf([tokenTypeId](const LegalizeRuleOperand &op)
                     { return op.m_instr->getOperands()[0]->getMirType()->getId() == tokenTypeId; })
            .dump();

    builder.begin(MirInstructionOpCode::POP_ARG)
            .minSize(1, i8)
            .expandIf([](const LegalizeRuleOperand &op)
                      { return op.m_instr->getOperands()[1]->getMirType()->getTotalSizeInBits() > 64; })
            .legalForDest(legalTypes)
            .dump();

    builder.begin(MirInstructionOpCode::POP_RET)
            .minSize(1, i8)
            .expandIf([](const LegalizeRuleOperand &op)
                      { return op.m_instr->getOperands()[1]->getMirType()->getTotalSizeInBits() > 64; })
            .legalIf([tokenTypeId](const LegalizeRuleOperand &op)
                     { return op.m_instr->getOperands()[0]->getMirType()->getId() == tokenTypeId; })
            .dump();

    builder.begin(MirInstructionOpCode::CALL)
            .legalIf([tokenTypeId](const LegalizeRuleOperand &op)
                     { return op.m_instr->getOperands()[0]->getMirType()->getId() == tokenTypeId; })
            .custom([tokenTypeId](const LegalizeRuleOperand &op)
                    { return op.m_instr->getOperands()[0]->getMirType()->getId() != tokenTypeId; },
                    legalizer->getCallAct())
            .dump();

    builder.begin(MirInstructionOpCode::RET)
            .legalIf(
                    [tokenTypeId](const LegalizeRuleOperand &op) {
                        return !op.m_instr->getOperands().empty() &&
                                op.m_instr->getOperands()[0]->getMirType()->getId() == tokenTypeId;
                    })
            .custom(
                    [tokenTypeId](const LegalizeRuleOperand &op) {
                        return !op.m_instr->getOperands().empty() &&
                                op.m_instr->getOperands()[0]->getMirType()->getId() != tokenTypeId;
                    },
                    legalizer->getReturnAct())
            .dump();

    builder.begin(MirInstructionOpCode::NOP).legalIf([](const LegalizeRuleOperand &) { return true; }).dump();
    builder.begin(MirInstructionOpCode::HALT).legalIf([](const LegalizeRuleOperand &) { return true; }).dump();
    builder.begin(MirInstructionOpCode::SYSCALL).legalIf([](const LegalizeRuleOperand &) { return true; }).dump();
}