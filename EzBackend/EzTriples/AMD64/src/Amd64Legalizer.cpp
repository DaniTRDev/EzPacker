#include "Amd64Legalizer.h"

std::unique_ptr<MirLegalizer> Amd64Legalizer::create(Amd64TargetDesc *targetDesc, MirBuilderContext *ctx)
{
    std::unique_ptr<MirLegalizer> legalizer = std::make_unique<MirLegalizer>(ctx, targetDesc);

    Amd64Legalizer instance;
    instance.addDataMovement(ctx, legalizer.get());
    instance.addMemory(ctx, legalizer.get());
    instance.addArithmetic(ctx, legalizer.get());
    instance.addBitwise(ctx, legalizer.get());
    instance.addCompare(ctx, legalizer.get());
    instance.addControlFlow(ctx, legalizer.get());
    instance.addCasting(ctx, legalizer.get());
    instance.addSystem(ctx, legalizer.get());

    return legalizer;
}

const std::vector<MirType *> Amd64Legalizer::getNativeSizes(MirBuilderContext *ctx) const
{
    const auto &t = ctx->getTypeTable();
    return { t->i8(), t->i16(), t->i32(), t->i64() };
}

void Amd64Legalizer::addDataMovement(MirBuilderContext *ctx, MirLegalizer *legalizer)
{
    const auto &t = ctx->getTypeTable();
    const auto &sizes = getNativeSizes(ctx);

    // Explicit token type ID used to group lowered stack / calling convention boundary lifecycles
    size_t tokenTypeId = t->getBindingToken()->getId();
    LegalizeAction *legal = MIRLEGALIZE_NO_ACTION;

    legalizer->addRuleForCategory(legal, MirCat_DataMovement, { t->i8()->getId(), MIRID_INVALID });
    legalizer->addRuleForCategory(legal, MirCat_DataMovement, { t->i16()->getId(), MIRID_INVALID });
    legalizer->addRuleForCategory(legal, MirCat_DataMovement, { t->i32()->getId(), MIRID_INVALID });
    legalizer->addRuleForCategory(legal, MirCat_DataMovement, { t->i64()->getId(), MIRID_INVALID });

    for (auto &dest : sizes)
    {
        size_t destId = dest->getId();
        // Modernized token-bound signatures: [Token, PayloadType]
        legalizer->addRule(legal, MirInstructionOpCode::PUSH_ARG, { tokenTypeId, destId });
        legalizer->addRule(legal, MirInstructionOpCode::POP_RET, { tokenTypeId, destId });
        legalizer->addRule(legal, MirInstructionOpCode::PUSH_RET, { tokenTypeId, destId });
    }
}

void Amd64Legalizer::addMemory(MirBuilderContext *ctx, MirLegalizer *legalizer)
{
    const auto &t = ctx->getTypeTable();
    LegalizeAction *legal = MIRLEGALIZE_NO_ACTION;

    // STORE: [Memory, Register/Immediate] -> Match destination data size
    legalizer->addRule(legal, MirInstructionOpCode::STORE, { MIRID_INVALID, t->i8()->getId() });
    legalizer->addRule(legal, MirInstructionOpCode::STORE, { MIRID_INVALID, t->i16()->getId() });
    legalizer->addRule(legal, MirInstructionOpCode::STORE, { MIRID_INVALID, t->i32()->getId() });
    legalizer->addRule(legal, MirInstructionOpCode::STORE, { MIRID_INVALID, t->i64()->getId() });

    // LOAD: [Register, Memory] -> Match source data size
    legalizer->addRule(legal, MirInstructionOpCode::LOAD, { t->i8()->getId(), MIRID_INVALID });
    legalizer->addRule(legal, MirInstructionOpCode::LOAD, { t->i16()->getId(), MIRID_INVALID });
    legalizer->addRule(legal, MirInstructionOpCode::LOAD, { t->i32()->getId(), MIRID_INVALID });
    legalizer->addRule(legal, MirInstructionOpCode::LOAD, { t->i64()->getId(), MIRID_INVALID });

    // CREATE (Stack allocation / Alloca): Destination gets pointer size (i64 on x64)
    legalizer->addRule(legal, MirInstructionOpCode::ALLOC, { t->i64()->getId() });
}

void Amd64Legalizer::addArithmetic(MirBuilderContext *ctx, MirLegalizer *legalizer)
{
    const auto &sizes = getNativeSizes(ctx);
    LegalizeAction *legal = MIRLEGALIZE_NO_ACTION;

    // x64 natively supports 8, 16, 32, and 64-bit scalar operations.
    for (const auto &type : sizes)
    {
        size_t sizeId = type->getId();
        legalizer->addRuleForCategory(legal, MirCat_Arithmetic, { sizeId, sizeId });
        // For unary operators like NEG which only have 1 operand:
        legalizer->addRule(legal, MirInstructionOpCode::NEG, { sizeId });
    }
}

void Amd64Legalizer::addBitwise(MirBuilderContext *ctx, MirLegalizer *legalizer)
{
    LegalizeAction *legal = MIRLEGALIZE_NO_ACTION;
    const auto &sizes = getNativeSizes(ctx);

    for (const auto &type : sizes)
    {
        size_t sizeId = type->getId();
        // AND, OR, XOR, SHL, SHR, SAR
        legalizer->addRuleForCategory(legal, MirCat_Bitwise, { sizeId, sizeId });
        // NOT is Unary
        legalizer->addRule(legal, MirInstructionOpCode::NOT, { sizeId });
    }
}

void Amd64Legalizer::addCompare(MirBuilderContext *ctx, MirLegalizer *legalizer)
{
    const auto &sizes = getNativeSizes(ctx);
    LegalizeAction *legal = MIRLEGALIZE_NO_ACTION;

    for (const auto &type : sizes)
    {
        size_t sizeId = type->getId();
        // CMP and TEST check two operands of identical size.
        legalizer->addRuleForCategory(legal, MirCat_Compare, { sizeId, sizeId });
    }
}

void Amd64Legalizer::addControlFlow(MirBuilderContext *ctx, MirLegalizer *legalizer)
{
    LegalizeAction *legal = MIRLEGALIZE_NO_ACTION;
    size_t tokenTypeId = ctx->getTypeTable()->getBindingToken()->getId();

    // Jumps, Branches (JE, JNE, etc.) take a reference.
    legalizer->addRuleForCategory(legal, MirCat_ControlFlow, { MIRID_INVALID });

    // Legal calls and returns explicitly target the system tracking token registration rule.
    // CALL layout: [Token, Callee]
    legalizer->addRule(legal, MirInstructionOpCode::CALL, { tokenTypeId, MIRID_INVALID });
    // RET layout:  [Token]
    legalizer->addRule(legal, MirInstructionOpCode::RET, { tokenTypeId });
}

void Amd64Legalizer::addCasting(MirBuilderContext *ctx, MirLegalizer *legalizer)
{
    const auto &sizes = getNativeSizes(ctx);
    LegalizeAction *legal = MIRLEGALIZE_NO_ACTION;

    for (const auto &dest : sizes)
    {
        size_t destId = dest->getId();
        legalizer->addRule(legal, MirInstructionOpCode::ZEXT, { destId, MIRID_INVALID });
        legalizer->addRule(legal, MirInstructionOpCode::SEXT, { destId, MIRID_INVALID });
        legalizer->addRule(legal, MirInstructionOpCode::TRUNC, { destId, MIRID_INVALID });
        legalizer->addRule(legal, MirInstructionOpCode::BITCAST, { destId, MIRID_INVALID });
    }
}

void Amd64Legalizer::addSystem(MirBuilderContext *ctx, MirLegalizer *legalizer)
{
    LegalizeAction *legal = MIRLEGALIZE_NO_ACTION;

    // SYSCALL, NOP, HALT take zero or ignored structural operands.
    legalizer->addRuleForCategory(legal, MirCat_System, {});
}