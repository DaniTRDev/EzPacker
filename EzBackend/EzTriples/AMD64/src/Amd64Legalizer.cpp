#include "Amd64Legalizer.h"

std::unique_ptr<MirLegalizer> Amd64Legalizer::create(MirBuilderContext *ctx)
{
    std::unique_ptr<MirLegalizer> legalizer = std::make_unique<MirLegalizer>(ctx->getDiagCollector().get());

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

const std::vector<size_t> Amd64Legalizer::getNativeSizes(MirBuilderContext *ctx) const
{
    const auto &t = ctx->getTypeTable();
    return { t->i8()->getId(), t->i16()->getId(), t->i32()->getId(), t->i64()->getId() };
}

void Amd64Legalizer::addDataMovement(MirBuilderContext *ctx, MirLegalizer *legalizer)
{
    const auto &t = ctx->getTypeTable();
    LegalizeAction *legal = MIRLEGALIZE_NO_ACTION;

    // MOV and LEA are completely legal natively across all 4 scalar sizes.
    legalizer->addRuleForCategory(legal, MirCat_DataMovement, { t->i8()->getId(), MIRID_INVALID });
    legalizer->addRuleForCategory(legal, MirCat_DataMovement, { t->i16()->getId(), MIRID_INVALID });
    legalizer->addRuleForCategory(legal, MirCat_DataMovement, { t->i32()->getId(), MIRID_INVALID });
    legalizer->addRuleForCategory(legal, MirCat_DataMovement, { t->i64()->getId(), MIRID_INVALID });
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
    legalizer->addRule(legal, MirInstructionOpCode::CREATE, { t->i64()->getId() });
}

void Amd64Legalizer::addArithmetic(MirBuilderContext *ctx, MirLegalizer *legalizer)
{
    const auto &sizes = getNativeSizes(ctx);
    LegalizeAction *legal = MIRLEGALIZE_NO_ACTION;
    
    // x64 natively supports 8, 16, 32, and 64-bit scalar operations.
    // Category mapping registers ADD, ADC, SUB, SBB, MUL, IMUL, DIV, IDIV, REM, NEG.
    // FloatingPoint-operand matching assumes { DestReg, SrcOperand }

    for (size_t sizeId : sizes)
    {
        legalizer->addRuleForCategory(legal, MirCat_Arithmetic, { sizeId, sizeId });
        // For unary operators like NEG which only have 1 operand:
        legalizer->addRule(legal, MirInstructionOpCode::NEG, { sizeId });
    }
}

void Amd64Legalizer::addBitwise(MirBuilderContext *ctx, MirLegalizer *legalizer)
{
    LegalizeAction *legal = MIRLEGALIZE_NO_ACTION;
    const auto &sizes = getNativeSizes(ctx);

    for (size_t sizeId : sizes)
    {
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
    
    for (size_t sizeId : sizes)
    {
        // CMP and TEST check two operands of identical size.
        legalizer->addRuleForCategory(legal, MirCat_Compare, { sizeId, sizeId });
    }
}

void Amd64Legalizer::addControlFlow(MirBuilderContext *ctx, MirLegalizer *legalizer)
{
    LegalizeAction *legal = MIRLEGALIZE_NO_ACTION;

    // Jumps, Branches (JE, JNE, etc.) take a reference.
    legalizer->addRuleForCategory(legal, MirCat_ControlFlow, { MIRID_INVALID });

    // CALL can take a Reference/Symbol or a register (i64 function pointer).
    legalizer->addRule(legal, MirInstructionOpCode::CALL, { MIRID_INVALID });
}

void Amd64Legalizer::addCasting(MirBuilderContext *ctx, MirLegalizer *legalizer)
{
    const auto &sizes = getNativeSizes(ctx);
    LegalizeAction *legal = MIRLEGALIZE_NO_ACTION;

    // Format: { DestType, SrcType }
    // Loop through combinations to permit valid structural legalizations
    for (size_t dest : sizes)
    {
        for (size_t src : sizes)
        {
            if (dest > src)
            {
                legalizer->addRule(legal, MirInstructionOpCode::ZEXT, { dest, src });
                legalizer->addRule(legal, MirInstructionOpCode::SEXT, { dest, src });
            }
            else if (dest < src)
            {
                legalizer->addRule(legal, MirInstructionOpCode::TRUNC, { dest, src });
            }
            else
            {
                legalizer->addRule(legal, MirInstructionOpCode::BITCAST, { dest, src });
            }
        }
    }
}

void Amd64Legalizer::addSystem(MirBuilderContext *ctx, MirLegalizer *legalizer)
{
    LegalizeAction *legal = MIRLEGALIZE_NO_ACTION;

    // SYSCALL, NOP, HALT take zero or ignored structural operands.
    legalizer->addRuleForCategory(legal, MirCat_System, {});
}
