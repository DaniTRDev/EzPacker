#include "Legalizer/EzTestTripleLegalizer.h"

namespace EzTestTriple
{
void CreateLegalizer(MirBuilderContext *ctx, MirLegalizer *legalizer)
{
    const auto &t = ctx->getTypeTable();

    // Cache target primitive types
    MirType *i8 = t->i8();
    MirType *i16 = t->i16();
    MirType *i32 = t->i32();
    MirType *i64 = t->i64();
    MirType *f32 = t->f32();
    MirType *f64 = t->f64();

    std::vector<MirType *> legalInts = { i8, i16, i32, i64 };
    std::vector<MirType *> legalFloats = { f32, f64 };
    std::vector<MirType *> legalScalars = { i8, i16, i32, i64, f32, f64 };
    std::vector<MirType *> legalWideInts = { i16, i32, i64 };

    LegalizeRuleBuilder builder(legalizer);

    // =========================================================================
    // 1. DATA MOVEMENT & MEMORY ACCESS
    // =========================================================================

    // MOV: Expand types > 64-bit into multi-slot moves; promote sub-i8 to i8
    builder.begin("MOV", MirInstructionOpCode::MOV)
            .expandIf(
                    [](const LegalizeCtx &lCtx)
                    {
                        MirInstruction *instr = *lCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return !ops.empty() && ops[0]->getMirType()->getTotalSizeInBits() > 64;
                    })
            .minSize(0, i8)
            .legalForDest(legalScalars);

    // LOAD: Expand > 64-bit chunk loads into discrete low/high loads
    builder.begin("LOAD", MirInstructionOpCode::LOAD)
            .expandIf(
                    [](const LegalizeCtx &lCtx)
                    {
                        MirInstruction *instr = *lCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return !ops.empty() && ops[0]->getMirType()->getTotalSizeInBits() > 64;
                    })
            .minSize(0, i8)
            .legalForDest(legalScalars);

    // STORE: Expand > 64-bit stores into discrete low/high stores
    builder.begin("STORE", MirInstructionOpCode::STORE)
            .expandIf(
                    [](const LegalizeCtx &lCtx)
                    {
                        MirInstruction *instr = *lCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return ops.size() > 1 && ops[1]->getMirType()->getTotalSizeInBits() > 64;
                    })
            .minSize(1, i8)
            .legalForSrc(legalScalars);

    // ALLOC: Stack frame variable slots must yield pointer types
    builder.begin("ALLOC", MirInstructionOpCode::ALLOC)
            .legalIf(
                    [](const LegalizeCtx &lCtx)
                    {
                        MirInstruction *instr = *lCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return !ops.empty() && ops[0]->getMirType()->getKind() == MirTypeKind::Pointer;
                    });

    // DALLOC: Dynamic alloca lowered by MirFrameLowererPass
    builder.begin("DALLOC", MirInstructionOpCode::DALLOC)
            .legalIf(
                    [](const LegalizeCtx &lCtx)
                    {
                        MirInstruction *instr = *lCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return ops.size() >= 2 && ops[0]->getMirType()->getKind() == MirTypeKind::Pointer;
                    });

    // =========================================================================
    // 2. STANDARD ARITHMETIC & BITWISE LOGIC (ALU)
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
                        [](const LegalizeCtx &lCtx)
                        {
                            MirInstruction *instr = *lCtx.m_it;
                            for (auto *operand : instr->getOperands())
                            {
                                if (operand->getMirType()->getTotalSizeInBits() > 64)
                                    return true;
                            }
                            return false;
                        })
                .minSize(0, i8)
                .minSize(1, i8)
                .legalForDest(legalInts);
    }

    // Floating-Point Arithmetic
    std::initializer_list<NamedOpCode> floatAluOps = { { "FADD", MirInstructionOpCode::FADD },
                                                       { "FSUB", MirInstructionOpCode::FSUB },
                                                       { "FMUL", MirInstructionOpCode::FMUL },
                                                       { "FDIV", MirInstructionOpCode::FDIV },
                                                       { "FCMP", MirInstructionOpCode::FCMP } };

    for (const auto &op : floatAluOps)
    {
        builder.begin(op.name, op.code).legalForDest(legalFloats);
    }

    // Multiply / Divide / Modulo (promoted to at least i16 for native hardware instructions)
    std::initializer_list<NamedOpCode> wideAluOps = { { "MUL", MirInstructionOpCode::MUL },
                                                      { "IMUL", MirInstructionOpCode::IMUL },
                                                      { "DIV", MirInstructionOpCode::DIV },
                                                      { "IDIV", MirInstructionOpCode::IDIV },
                                                      { "REM", MirInstructionOpCode::REM } };

    for (const auto &op : wideAluOps)
    {
        builder.begin(op.name, op.code)
                .expandIf(
                        [](const LegalizeCtx &lCtx)
                        {
                            MirInstruction *instr = *lCtx.m_it;
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

    // Unary Operations
    builder.begin("NEG", MirInstructionOpCode::NEG)
            .expandIf(
                    [](const LegalizeCtx &lCtx)
                    {
                        MirInstruction *instr = *lCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return !ops.empty() && ops[0]->getMirType()->getTotalSizeInBits() > 64;
                    })
            .minSize(0, i8)
            .legalForDest(legalInts);

    builder.begin("NOT", MirInstructionOpCode::NOT)
            .expandIf(
                    [](const LegalizeCtx &lCtx)
                    {
                        MirInstruction *instr = *lCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return !ops.empty() && ops[0]->getMirType()->getTotalSizeInBits() > 64;
                    })
            .minSize(0, i8)
            .legalForDest(legalInts);

    // Shifts & Rotates
    std::initializer_list<NamedOpCode> shiftOps = { { "SHL", MirInstructionOpCode::SHL },
                                                    { "SHR", MirInstructionOpCode::SHR },
                                                    { "SAR", MirInstructionOpCode::SAR } };

    for (const auto &op : shiftOps)
    {
        builder.begin(op.name, op.code)
                .expandIf(
                        [](const LegalizeCtx &lCtx)
                        {
                            MirInstruction *instr = *lCtx.m_it;
                            const auto &ops = instr->getOperands();
                            return !ops.empty() && ops[0]->getMirType()->getTotalSizeInBits() > 64;
                        })
                .minSize(0, i8)
                .minSize(1, i8)
                .legalForDest(legalInts);
    }

    // =========================================================================
    // 3. CASTING & CONVERSIONS
    // =========================================================================
    builder.begin("ZEXT", MirInstructionOpCode::ZEXT).legalForDest(legalInts);
    builder.begin("SEXT", MirInstructionOpCode::SEXT).legalForDest(legalInts);
    builder.begin("TRUNC", MirInstructionOpCode::TRUNC).legalForDest(legalInts);
    builder.begin("FPEXT", MirInstructionOpCode::FPEXT).legalForDest({ f64 });
    builder.begin("SITOFP", MirInstructionOpCode::SITOFP).legalForDest(legalFloats);
    builder.begin("FPTOSI", MirInstructionOpCode::FPTOSI).legalForDest(legalInts);
    builder.begin("BITCAST", MirInstructionOpCode::BITCAST).legalForDest(legalScalars);

    // =========================================================================
    // 4. CONTROL FLOW & BRANCHES
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

    // =========================================================================
    // 5. CALLING CONVENTION ABI LOWERING (PUSH_ARG, POP_ARG, CALL, RET)
    // =========================================================================
    size_t tokenTypeId = t->getBindingToken()->getId();

    builder.begin("PUSH_ARG", MirInstructionOpCode::PUSH_ARG)
            .minSize(1, i8)
            .expandIf(
                    [](const LegalizeCtx &lCtx)
                    {
                        MirInstruction *instr = *lCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return ops.size() > 1 && ops[1]->getMirType()->getTotalSizeInBits() > 64;
                    })
            .legalIf(
                    [tokenTypeId](const LegalizeCtx &lCtx)
                    {
                        MirInstruction *instr = *lCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return !ops.empty() && ops[0]->getMirType()->getId() == tokenTypeId;
                    });

    builder.begin("POP_ARG", MirInstructionOpCode::POP_ARG)
            .minSize(1, i8)
            .expandIf(
                    [](const LegalizeCtx &lCtx)
                    {
                        MirInstruction *instr = *lCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return ops.size() > 1 && ops[1]->getMirType()->getTotalSizeInBits() > 64;
                    })
            .legalForDest(legalScalars);

    builder.begin("PUSH_RET", MirInstructionOpCode::PUSH_RET)
            .minSize(1, i8)
            .expandIf(
                    [](const LegalizeCtx &lCtx)
                    {
                        MirInstruction *instr = *lCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return ops.size() > 1 && ops[1]->getMirType()->getTotalSizeInBits() > 64;
                    })
            .legalIf(
                    [tokenTypeId](const LegalizeCtx &lCtx)
                    {
                        MirInstruction *instr = *lCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return !ops.empty() && ops[0]->getMirType()->getId() == tokenTypeId;
                    });

    builder.begin("POP_RET", MirInstructionOpCode::POP_RET)
            .minSize(1, i8)
            .expandIf(
                    [](const LegalizeCtx &lCtx)
                    {
                        MirInstruction *instr = *lCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return ops.size() > 1 && ops[1]->getMirType()->getTotalSizeInBits() > 64;
                    })
            .legalIf(
                    [tokenTypeId](const LegalizeCtx &lCtx)
                    {
                        MirInstruction *instr = *lCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return !ops.empty() && ops[0]->getMirType()->getId() == tokenTypeId;
                    });

    // CALL & RET: Require custom legalization if token binding has not occurred yet
    builder.begin("CALL", MirInstructionOpCode::CALL)
            .legalIf(
                    [tokenTypeId](const LegalizeCtx &lCtx)
                    {
                        MirInstruction *instr = *lCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return !ops.empty() && ops[0]->getMirType()->getId() == tokenTypeId;
                    })
            .custom(
                    [tokenTypeId](const LegalizeCtx &lCtx)
                    {
                        MirInstruction *instr = *lCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return ops.empty() || ops[0]->getMirType()->getId() != tokenTypeId;
                    },
                    LegalizeActions::LegalizeCall);

    builder.begin("RET", MirInstructionOpCode::RET)
            .legalIf(
                    [tokenTypeId](const LegalizeCtx &lCtx)
                    {
                        MirInstruction *instr = *lCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return !ops.empty() && ops[0]->getMirType()->getId() == tokenTypeId;
                    })
            .custom(
                    [tokenTypeId](const LegalizeCtx &lCtx)
                    {
                        MirInstruction *instr = *lCtx.m_it;
                        const auto &ops = instr->getOperands();
                        return ops.empty() || ops[0]->getMirType()->getId() != tokenTypeId;
                    },
                    LegalizeActions::LegalizeReturn);

    // =========================================================================
    // 6. SYSTEM & NO-OP INSTRUCTIONS
    // =========================================================================
    builder.begin("NOP", MirInstructionOpCode::NOP).legalIf([](const LegalizeCtx &) { return true; });
    builder.begin("HALT", MirInstructionOpCode::HALT).legalIf([](const LegalizeCtx &) { return true; });
    builder.begin("SYSCALL", MirInstructionOpCode::SYSCALL).legalIf([](const LegalizeCtx &) { return true; });
}
}; // namespace EzTestTriple