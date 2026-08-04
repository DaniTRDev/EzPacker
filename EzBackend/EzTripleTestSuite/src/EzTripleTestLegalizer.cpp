#include "../include/EzTripleTestLegalizer.h"

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

    std::vector<MirType *> legalTypes = { i8, i16, i32, i64, f32, f64 };
    std::vector<MirType *> legalInts = { i8, i16, i32, i64 };
    std::vector<MirType *> legalWideInts = { i16, i32, i64 };

    // Instantiate the stateful rule builder
    LegalizeRuleBuilder builder(legalizer);

    // =========================================================================
    // 1. DATA MOVEMENT & MEMORY ACCESS
    // =========================================================================
    builder.begin("MOV", MirInstructionOpCode::MOV)
            .minSize(0, i8) // Intercept small types first!
            .legalForDest(legalTypes);

    builder.begin("LOAD", MirInstructionOpCode::LOAD)
            .expandIf(
                    [](const LegalizeCtx &legalizeCtx)
                    {
                        MirInstruction *instr = *legalizeCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return !ops.empty() && ops[0]->getMirType()->getTotalSizeInBits() > 64;
                    })
            .minSize(0, i8)
            .legalForDest(legalTypes);

    builder.begin("STORE", MirInstructionOpCode::STORE)
            .expandIf(
                    [](const LegalizeCtx &legalizeCtx)
                    {
                        MirInstruction *instr = *legalizeCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return ops.size() > 1 && ops[1]->getMirType()->getTotalSizeInBits() > 64;
                    })
            .minSize(1, i8)
            .legalForSrc(legalTypes);

    builder.begin("ALLOC", MirInstructionOpCode::ALLOC)
            .legalIf(
                    [](const LegalizeCtx &legalizeCtx)
                    {
                        MirInstruction *instr = *legalizeCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return !ops.empty() && ops[0]->getMirType()->getKind() == MirTypeKind::Pointer;
                    });

    // =========================================================================
    // 2. ARITHMETIC & LOGIC (ALU)
    // =========================================================================
    struct NamedOpCode
    {
        const char *name;
        MirInstructionOpCode code;
    };

    std::initializer_list<NamedOpCode> standardAluOps = {
        { "ADD", MirInstructionOpCode::ADD },   { "SUB", MirInstructionOpCode::SUB },
        { "AND", MirInstructionOpCode::AND },   { "OR", MirInstructionOpCode::OR },
        { "XOR", MirInstructionOpCode::XOR },   { "CMP", MirInstructionOpCode::CMP },
        { "TEST", MirInstructionOpCode::TEST }, { "ADC", MirInstructionOpCode::ADC },
        { "SBB", MirInstructionOpCode::SBB }
    };

    for (const auto &op : standardAluOps)
    {
        builder.begin(op.name, op.code)
                .expandIf(
                        [](const LegalizeCtx &legalizeCtx)
                        {
                            // 1. Force scalar expansion if any operand exceeds native 64-bit bounds
                            MirInstruction *instr = *legalizeCtx.m_it;
                            for (auto *operand : instr->getOperands())
                            {
                                if (operand->getMirType()->getTotalSizeInBits() > 64)
                                    return true;
                            }
                            return false;
                        })
                .minSize(0, i8)          // 2. Catch and promote if dest is under-sized
                .minSize(1, i8)          // 3. Intercept and promote if source (operand 1) is under-sized
                .legalForDest(legalInts) // 4. Only accept native sizes if everything else passes
                ;
    }

    std::initializer_list<NamedOpCode> wideAluOps = { { "MUL", MirInstructionOpCode::MUL },
                                                      { "IMUL", MirInstructionOpCode::IMUL },
                                                      { "DIV", MirInstructionOpCode::DIV },
                                                      { "IDIV", MirInstructionOpCode::IDIV },
                                                      { "REM", MirInstructionOpCode::REM } };

    for (const auto &op : wideAluOps)
    {
        builder.begin(op.name, op.code)
                .expandIf(
                        [](const LegalizeCtx &legalizeCtx)
                        {
                            MirInstruction *instr = *legalizeCtx.m_it;
                            for (auto *operand : instr->getOperands())
                            {
                                if (operand->getMirType()->getTotalSizeInBits() > 64)
                                    return true;
                            }
                            return false;
                        })
                .minSize(0, i16)
                .minSize(1, i16)
                .legalForDest(legalWideInts);
    }

    builder.begin("NEG", MirInstructionOpCode::NEG)
            .expandIf(
                    [](const LegalizeCtx &legalizeCtx)
                    {
                        MirInstruction *instr = *legalizeCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return !ops.empty() && ops[0]->getMirType()->getTotalSizeInBits() > 64;
                    })
            .minSize(0, i8)
            .legalForDest(legalInts);

    builder.begin("NOT", MirInstructionOpCode::NOT)
            .expandIf(
                    [](const LegalizeCtx &legalizeCtx)
                    {
                        MirInstruction *instr = *legalizeCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return !ops.empty() && ops[0]->getMirType()->getTotalSizeInBits() > 64;
                    })
            .minSize(0, i8)
            .legalForDest(legalInts);

    std::initializer_list<NamedOpCode> shiftOps = { { "SHL", MirInstructionOpCode::SHL },
                                                    { "SHR", MirInstructionOpCode::SHR },
                                                    { "SAR", MirInstructionOpCode::SAR } };

    for (const auto &op : shiftOps)
    {
        builder.begin(op.name, op.code).minSize(0, i8).minSize(1, i8).legalForDest(legalInts);
    }

    // =========================================================================
    // 3. CASTING & EXTENSIONS
    // =========================================================================
    builder.begin("ZEXT", MirInstructionOpCode::ZEXT).legalForDest(legalInts);
    builder.begin("SEXT", MirInstructionOpCode::SEXT).legalForDest(legalInts);
    builder.begin("TRUNC", MirInstructionOpCode::TRUNC).legalForDest(legalInts);
    builder.begin("FPEXT", MirInstructionOpCode::FPEXT).legalForDest({ f64 });
    builder.begin("BITCAST", MirInstructionOpCode::BITCAST).legalForDest(legalTypes);

    // =========================================================================
    // 4. CONTROL FLOW & SYSTEM RULES
    // =========================================================================
    std::initializer_list<NamedOpCode> branches = {
        { "JMP", MirInstructionOpCode::JMP }, { "JE", MirInstructionOpCode::JE },
        { "JNE", MirInstructionOpCode::JNE }, { "JG", MirInstructionOpCode::JG },
        { "JGE", MirInstructionOpCode::JGE }, { "JL", MirInstructionOpCode::JL },
        { "JLE", MirInstructionOpCode::JLE }, { "JA", MirInstructionOpCode::JA },
        { "JB", MirInstructionOpCode::JB }
    };

    for (const auto &op : branches)
    {
        builder.begin(op.name, op.code).legalIf([](const LegalizeCtx &) { return true; });
    }

    size_t tokenTypeId = t->getBindingToken()->getId();

    builder.begin("PUSH_ARG", MirInstructionOpCode::PUSH_ARG)
            .minSize(1, i8)
            .expandIf(
                    [](const LegalizeCtx &legalizeCtx)
                    {
                        MirInstruction *instr = *legalizeCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return ops.size() > 1 && ops[1]->getMirType()->getTotalSizeInBits() > 64;
                    })
            .legalIf(
                    [tokenTypeId](const LegalizeCtx &legalizeCtx)
                    {
                        MirInstruction *instr = *legalizeCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return !ops.empty() && ops[0]->getMirType()->getId() == tokenTypeId;
                    });

    builder.begin("PUSH_RET", MirInstructionOpCode::PUSH_RET)
            .minSize(1, i8)
            .expandIf(
                    [](const LegalizeCtx &legalizeCtx)
                    {
                        MirInstruction *instr = *legalizeCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return ops.size() > 1 && ops[1]->getMirType()->getTotalSizeInBits() > 64;
                    })
            .legalIf(
                    [tokenTypeId](const LegalizeCtx &legalizeCtx)
                    {
                        MirInstruction *instr = *legalizeCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return !ops.empty() && ops[0]->getMirType()->getId() == tokenTypeId;
                    });

    builder.begin("POP_ARG", MirInstructionOpCode::POP_ARG)
            .minSize(1, i8)
            .expandIf(
                    [](const LegalizeCtx &legalizeCtx)
                    {
                        MirInstruction *instr = *legalizeCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return ops.size() > 1 && ops[1]->getMirType()->getTotalSizeInBits() > 64;
                    })
            .legalForDest(legalTypes);

    builder.begin("POP_RET", MirInstructionOpCode::POP_RET)
            .minSize(1, i8)
            .expandIf(
                    [](const LegalizeCtx &legalizeCtx)
                    {
                        MirInstruction *instr = *legalizeCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return ops.size() > 1 && ops[1]->getMirType()->getTotalSizeInBits() > 64;
                    })
            .legalIf(
                    [tokenTypeId](const LegalizeCtx &legalizeCtx)
                    {
                        MirInstruction *instr = *legalizeCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return !ops.empty() && ops[0]->getMirType()->getId() == tokenTypeId;
                    });

    builder.begin("CALL", MirInstructionOpCode::CALL)
            .legalIf(
                    [tokenTypeId](const LegalizeCtx &legalizeCtx)
                    {
                        MirInstruction *instr = *legalizeCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return !ops.empty() && ops[0]->getMirType()->getId() == tokenTypeId;
                    })
            .custom(
                    [tokenTypeId](const LegalizeCtx &legalizeCtx)
                    {
                        MirInstruction *instr = *legalizeCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return ops.empty() || ops[0]->getMirType()->getId() != tokenTypeId;
                    },
                    LegalizeActions::LegalizeCall);

    builder.begin("RET", MirInstructionOpCode::RET)
            .legalIf(
                    [tokenTypeId](const LegalizeCtx &legalizeCtx)
                    {
                        MirInstruction *instr = *legalizeCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return !ops.empty() && ops[0]->getMirType()->getId() == tokenTypeId;
                    })
            .custom(
                    [tokenTypeId](const LegalizeCtx &legalizeCtx)
                    {
                        MirInstruction *instr = *legalizeCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return ops.empty() || ops[0]->getMirType()->getId() != tokenTypeId;
                    },
                    LegalizeActions::LegalizeReturn);

    builder.begin("NOP", MirInstructionOpCode::NOP).legalIf([](const LegalizeCtx &) { return true; });
    builder.begin("HALT", MirInstructionOpCode::HALT).legalIf([](const LegalizeCtx &) { return true; });
    builder.begin("SYSCALL", MirInstructionOpCode::SYSCALL).legalIf([](const LegalizeCtx &) { return true; });
}