#include "Amd64Legalizer.h"

std::unique_ptr<MirLegalizer> Amd64Legalizer::create(MirBuilderContext *ctx)
{
    std::unique_ptr<MirLegalizer> legalizer = std::make_unique<MirLegalizer>(ctx->getDiagCollector().get());

    return legalizer;
}

void Amd64Legalizer::addDataMovement(MirBuilderContext *ctx, MirLegalizer *legalizer)
{
    LegalizeAction *legal = MIRLEGALIZE_NO_ACTION;
    const auto &t = ctx->getTypeTable();

    // MOV and LEA.
    legalizer->addRuleForCategory(legal, MirCat_DataMovement, { t->i8()->getId(), MIRID_INVALID });
    legalizer->addRuleForCategory(legal, MirCat_DataMovement, { t->i16()->getId(), MIRID_INVALID });
    legalizer->addRuleForCategory(legal, MirCat_DataMovement, { t->i32()->getId(), MIRID_INVALID });
    legalizer->addRuleForCategory(legal, MirCat_DataMovement, { t->i64()->getId(), MIRID_INVALID });
}

void Amd64Legalizer::addMemory(MirBuilderContext *ctx, MirLegalizer *legalizer)
{
    LegalizeAction *expansion = nullptr, *promotion = nullptr, *legal = MIRLEGALIZE_NO_ACTION;
    const auto &t = ctx->getTypeTable();

    // STORE
    legalizer->addRule(legal, MirInstructionOpCode::STORE, { t->i8()->getId(), MIRID_INVALID });
    legalizer->addRule(legal, MirInstructionOpCode::STORE, { t->i16()->getId(), MIRID_INVALID });
    legalizer->addRule(legal, MirInstructionOpCode::STORE, { t->i32()->getId(), MIRID_INVALID });
    legalizer->addRule(legal, MirInstructionOpCode::STORE, { t->i64()->getId(), MIRID_INVALID });

    // LOAD
    legalizer->addRule(legal, MirInstructionOpCode::STORE, { MIRID_INVALID, t->i8()->getId() });
    legalizer->addRule(legal, MirInstructionOpCode::STORE, { MIRID_INVALID, t->i16()->getId() });
    legalizer->addRule(legal, MirInstructionOpCode::STORE, { MIRID_INVALID, t->i32()->getId() });
    legalizer->addRule(legal, MirInstructionOpCode::STORE, { MIRID_INVALID, t->i64()->getId() });

    // CREATE
    legalizer->addRule(legal, MirInstructionOpCode::CREATE, { MIRID_INVALID });
}
