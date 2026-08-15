#include "Legalizer/Actions/ExpandScalarAction.h"
#include "Legalizer/Expand/MirExpansionRuleRegistry.h"
#include <unordered_map>
#include <format>

namespace LegalizeActions
{
LegalizationResult ExpandScalar(LegalizeCtx &ctx)
{
    auto it = ctx.m_it;
    MirBuilderContext *builderCtx = ctx.m_ctx;
    MirInstruction *instr = *it;
    auto &operands = instr->getOperands();
    auto &expandMap = ctx.m_expandMap;

    const auto &typeTable = builderCtx->getTypeTable();

    // 1. Context Boundaries & Payload Identification
    bool op0IsToken =
            (!operands.empty()) && (operands[0]->getMirType()->getId() == typeTable->getBindingToken()->getId());

    size_t payloadIdx = op0IsToken ? 1 : 0;
    MirType *fullType = operands[payloadIdx]->getMirType();
    MirType *halfType = typeTable->getIntegerTypeBySize(fullType->getTotalSizeInBits() / 2);

    // 2. Query Rule Registry via ExpansionContext
    ExpansionContext expCtx{ .m_expandedType = halfType, .m_fullType = fullType, .m_it = it };

    MirExpansionRuleRegistry *ruleRegistry = ctx.m_targetDesc->getExpansionRegistry();
    if (!ruleRegistry)
    {
        return LegalizationResult::AlreadyLegal;
    }

    ExpansionRule *rule = ruleRegistry->getRule(expCtx);
    if (!rule)
    {
        return LegalizationResult::AlreadyLegal;
    }

    builderCtx->getDiagCollector()->builder(Diag_Trace, "ExpandScalarAction")
            << std::format("Expanding wide type '{}' into halves of '{}'", fullType->getName(), halfType->getName())
                       .c_str()
            << instr->getSourceRef();

    MirOperandBuilder opBuilder(builderCtx);
    MirInstructionBuilder insertBeforeBuilder(builderCtx, instr->getOwner(), InsertionType::InsertBefore, it);

    MirOperand *destLo = nullptr, *destHi = nullptr, *srcLo = nullptr, *srcHi = nullptr;

    // 3. Destination Operand Splitting
    if (!op0IsToken)
    {
        if (operands[0]->isOfType<MirRegister>())
        {
            MirRegister *dest = operands[0]->get<MirRegister>();
            auto destIt = expandMap.find(dest->getRegId());

            if (destIt != expandMap.end())
            {
                destLo = destIt->second.first;
                destHi = destIt->second.second;

                builderCtx->getDiagCollector()->builder(Diag_Trace, "ExpandScalarAction")
                        << std::format("Reusing split dst register '{}', '{}' and '{}'",
                                       dest->getName(),
                                       destLo->get<MirRegister>()->getName(),
                                       destHi->get<MirRegister>()->getName())
                                   .c_str()
                        << dest->getSourceRef();
            }
            else
            {
                destLo = opBuilder.buildVReg(halfType, dest->getName() + "_lo", dest->getSourceRef());
                destHi = opBuilder.buildVReg(halfType, dest->getName() + "_hi", dest->getSourceRef());

                builderCtx->getDiagCollector()->builder(Diag_Trace, "ExpandScalarAction")
                        << std::format("Split dst register '{}' into '{}' and '{}'",
                                       dest->getName(),
                                       destLo->get<MirRegister>()->getName(),
                                       destHi->get<MirRegister>()->getName())
                                   .c_str()
                        << dest->getSourceRef();

                expandMap[dest->getRegId()] = std::make_pair((MirRegister *)destLo, (MirRegister *)destHi);
            }
        }
        else
        {
            destLo = operands[0];
        }
    }
    else
    {
        // Preserve binding token intact
        destLo = operands[0];
    }

    // 4. Source Operand Splitting
    size_t sourceIdx = op0IsToken ? 1 : 1;
    bool singleOperand = !op0IsToken && (operands.size() == 1);

    if (op0IsToken || operands.size() > 1 || singleOperand)
    {
        MirOperand *source = singleOperand ? operands[0] : operands[sourceIdx];

        if (source->isOfType<MirRegister>())
        {
            MirRegister *r = source->get<MirRegister>();
            auto srcIt = expandMap.find(r->getRegId());

            if (srcIt != expandMap.end())
            {
                srcLo = srcIt->second.first;
                srcHi = srcIt->second.second;

                builderCtx->getDiagCollector()->builder(Diag_Trace, "ExpandScalarAction")
                        << std::format("Reusing split src register '{}', '{}' and '{}'",
                                       r->getName(),
                                       srcLo->get<MirRegister>()->getName(),
                                       srcHi->get<MirRegister>()->getName())
                                   .c_str()
                        << r->getSourceRef();
            }
            else
            {
                if (r->getMirType()->getTotalSizeInBits() > halfType->getTotalSizeInBits())
                {
                    srcLo = opBuilder.buildVReg(halfType, r->getName() + "_lo", source->getSourceRef());
                    srcHi = opBuilder.buildVReg(halfType, r->getName() + "_hi", source->getSourceRef());

                    builderCtx->getDiagCollector()->builder(Diag_Trace, "ExpandScalarAction")
                            << std::format("Split src register '{}' into '{}' and '{}'",
                                           r->getName(),
                                           srcLo->get<MirRegister>()->getName(),
                                           srcHi->get<MirRegister>()->getName())
                                       .c_str()
                            << source->getSourceRef();

                    expandMap[r->getRegId()] = std::make_pair((MirRegister *)srcLo, (MirRegister *)srcHi);
                }
                else
                {
                    srcLo = source;
                    srcHi = nullptr;
                }
            }
        }
        else if (source->isOfType<MirInteger>())
        {
            auto val = source->get<MirInteger>()->getValue();
            srcLo = opBuilder.buildInt(halfType, val.getLowHalf(), source->getSourceRef());
            srcHi = opBuilder.buildInt(halfType, val.getHighHalf(), source->getSourceRef());

            builderCtx->getDiagCollector()->builder(Diag_Trace, "ExpandScalarAction")
                    << std::format("Split integer literal value '{}' into '{}' and '{}'",
                                   source->toString(),
                                   srcLo->get<MirInteger>()->getValue().toString(16),
                                   srcHi->get<MirInteger>()->getValue().toString(16))
                               .c_str()
                    << source->getSourceRef();
        }
        else if (source->isOfType<MirFloat>())
        {
            builderCtx->getDiagCollector()->builder(Diag_Error, "ExpandScalarAction")
                    << "Can't expand floating point instructions" << instr->getSourceRef();
            return LegalizationResult::LegalizationError;
        }
        else
        {
            srcLo = source;
            srcHi = nullptr;
        }
    }

    // 5. Unbounded Temporal Register Allocation Map
    // Key formula: (id << 1) | (isHigh ? 1 : 0)
    std::unordered_map<uint64_t, MirOperand *> temporalRegs;

    auto getTemporalRegister = [&](size_t id, bool isHigh) -> MirOperand *
    {
        uint64_t key = (static_cast<uint64_t>(id) << 1) | (isHigh ? 1 : 0);
        auto tempIt = temporalRegs.find(key);
        if (tempIt != temporalRegs.end())
        {
            return tempIt->second;
        }

        std::string name = std::format("t{}_{}", id, isHigh ? "hi" : "lo");
        MirOperand *vReg = opBuilder.buildVReg(halfType, name.c_str(), instr->getSourceRef());
        temporalRegs[key] = vReg;
        return vReg;
    };

    // 6. Sentence Interpretation Loop
    for (const auto &expInstr : rule->m_instructions)
    {
        std::vector<MirOperand *> newOps;
        newOps.reserve(expInstr.m_operands.size());

        for (const auto &recipeOp : expInstr.m_operands)
        {
            MirOperand *resolvedOp = nullptr;
            switch (recipeOp.m_type)
            {
                case ExpansionOperandType::DestLow:
                    resolvedOp = destLo;
                    break;
                case ExpansionOperandType::DestHigh:
                    resolvedOp = op0IsToken ? nullptr : destHi;
                    break;
                case ExpansionOperandType::SrcLow:
                    resolvedOp = srcLo;
                    break;
                case ExpansionOperandType::SrcHigh:
                    resolvedOp = srcHi;
                    break;

                case ExpansionOperandType::Temporal:
                    resolvedOp = getTemporalRegister(recipeOp.m_tempOperand.m_id, recipeOp.m_tempOperand.m_high);
                    break;

                case ExpansionOperandType::IntImm:
                    resolvedOp = opBuilder.buildInt(halfType, recipeOp.intVal);
                    break;

                case ExpansionOperandType::FloatImm:
                    resolvedOp = opBuilder.buildFloat(halfType, recipeOp.floatVal);
                    break;

                case ExpansionOperandType::MemoryHalfOffset:
                {
                    MirOperand *baseOp =
                            (recipeOp.m_memOperand.m_base == ExpansionOperandType::SrcLow) ? srcLo : destLo;

                    int64_t halfSizeInBytes = static_cast<int64_t>(halfType->getTotalSizeInBits() / 8);
                    int64_t stride = recipeOp.m_memOperand.m_scaleHalfFactor * halfSizeInBytes;
                    int64_t finalOffset = stride + recipeOp.m_memOperand.m_displ;

                    if (baseOp && baseOp->isOfType<MirMemory>())
                    {
                        MirMemory *origMem = baseOp->get<MirMemory>();
                        FlexInt origDispl = origMem->getDisplacement()->getValue();
                        FlexInt newDispl = origDispl + FlexInt(finalOffset, origDispl.getBitSize());

                        resolvedOp =
                                opBuilder.buildMem(halfType, origMem->getBase(), newDispl, origMem->getSourceRef());
                    }
                    else if (baseOp && baseOp->isOfType<MirRegister>())
                    {
                        resolvedOp = opBuilder.buildMem(halfType,
                                                        baseOp->get<MirRegister>(),
                                                        FlexInt(finalOffset, 64),
                                                        baseOp->getSourceRef());
                    }
                    break;
                }
            }

            if (resolvedOp)
            {
                newOps.push_back(resolvedOp);
            }
        }

        if (!expInstr.m_rtLibraryCall.empty())
        {
            newOps.insert(newOps.begin(),
                          opBuilder.buildRtSymbol(expInstr.m_rtLibraryCall.c_str(), instr->getSourceRef()));
        }

        insertBeforeBuilder.build(expInstr.m_opcode, instr->getSourceRef(), newOps);
    }

    // Erase original unexpanded instruction
    ctx.m_instrList->erase(it);
    return LegalizationResult::Legalized;
}
} // namespace LegalizeActions