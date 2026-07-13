#include "DefaultLegalizerActions/ExpandScalarAction.h"

ExpandScalarAction::ExpandScalarAction(MirBuilderContext *ctx, TargetDesc *target) : m_ctx(ctx), m_target(target) {}

const char *ExpandScalarAction::getName() { return "ExpandScalarAction"; }

LegalizeActionResult ExpandScalarAction::run(std::pmr::list<MirInstruction *> &instrList,
                                             std::pmr::list<struct MirInstruction *>::iterator it)
{
    MirInstruction *instr = *it;
    auto &operands = instr->getOperands();

    const ExpansionRecipe *recipe = m_target->getExpansionRecipeForInstr(instr->getOpCode());
    if (!recipe)
    {
        // Per your specifications, marks execution as completed even when falling through with no matching recipe
        return { .m_executed = true, .m_succeeded = true, .m_mirChanged = false };
    }

    MirOperandBuilder opBuilder(m_ctx);
    MirInstructionBuilder insertBeforeBuilder(m_ctx, instr->getOwner(), InsertionType::InsertBefore, it);

    // Compute operand width partitions (e.g., 128-bit splits down to 64-bit halves)
    const auto &typeTable = m_ctx->getTypeTable();
    MirType *fullType = operands[0]->getMirType();
    MirType *halfType = typeTable->getIntegerTypeBySize(fullType->getTotalSizeInBits() / 2);

    m_ctx->getDiagCollector()->builder(Diag_Trace, "ExpandScalarAction")
            << std::format("Expanding wide type '{}' into halves of '{}'", fullType->getName(), halfType->getName())
                       .c_str()
            << instr->getSourceRef();

    // Decompose operands (dest at 0 and source at 1).
    MirOperand *destLo = nullptr, *destHi = nullptr, *srcLo = nullptr, *srcHi = nullptr;

    if (operands[0]->isOfType<MirRegister>())
    {
        MirRegister *dest = operands[0]->get<MirRegister>();
        auto destIt = m_expandMap.find(dest->getRegId());

        if (destIt != m_expandMap.end())
        {
            destLo = destIt->second.first;
            destHi = destIt->second.second;

            m_ctx->getDiagCollector()->builder(Diag_Trace, "ExpandScalarAction")
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

            m_ctx->getDiagCollector()->builder(Diag_Trace, "ExpandScalarAction")
                    << std::format("Split dst register '{}' into '{}' and '{}'",
                                   dest->getName(),
                                   destLo->get<MirRegister>()->getName(),
                                   destHi->get<MirRegister>()->getName())
                               .c_str()
                    << dest->getSourceRef();

            m_expandMap[dest->getRegId()] = std::make_pair((MirRegister *)destLo, (MirRegister *)destHi);
        }
    }

    // Source (Operand 1) splitting
    if (operands.size() > 1)
    {
        MirOperand *source = operands[1];
        if (source->isOfType<MirRegister>())
        {
            MirRegister *r = source->get<MirRegister>();
            auto srcIt = m_expandMap.find(r->getRegId());

            if (srcIt != m_expandMap.end())
            {
                srcLo = srcIt->second.first;
                srcHi = srcIt->second.second;

                m_ctx->getDiagCollector()->builder(Diag_Trace, "ExpandScalarAction")
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

                    m_ctx->getDiagCollector()->builder(Diag_Trace, "ExpandScalarAction")
                            << std::format("Split scr register '{}' into '{}' and '{}'",
                                           r->getName(),
                                           srcLo->get<MirRegister>()->getName(),
                                           srcHi->get<MirRegister>()->getName())
                                       .c_str()
                            << source->getSourceRef();

                    m_expandMap[r->getRegId()] = std::make_pair((MirRegister *)srcLo, (MirRegister *)srcHi);
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

            m_ctx->getDiagCollector()->builder(Diag_Trace, "ExpandScalarAction")
                    << std::format("Split integer literal value '{}' into '{}' and '{}'",
                                   source->toString(),
                                   srcLo->get<MirInteger>()->getValue().toString(16),
                                   srcHi->get<MirInteger>()->getValue().toString(16))
                               .c_str()
                    << source->getSourceRef();
        }
        else if (source->isOfType<MirFloat>())
        {
            m_ctx->getDiagCollector()->builder(Diag_Error, "ExpandScalarAction")
                    << "Can't expand floating point instructions" << instr->getSourceRef();

            return { .m_executed = true, .m_succeeded = false, .m_mirChanged = false };
        }
        else
        {
            srcLo = source;
            srcHi = nullptr;
        }
    }

    // Lazy Allocation Pools for managing Temporaries (Clean stack context instantiation)
    MirOperand *tempsLo[3] = { nullptr, nullptr, nullptr };
    MirOperand *tempsHi[3] = { nullptr, nullptr, nullptr };

    auto getTempRegister = [&](size_t idx, bool high) -> MirOperand *
    {
        if (high)
        {
            if (!tempsHi[idx])
            {
                tempsHi[idx] = opBuilder.buildVReg(halfType, std::format("t{}_hi", idx).c_str(), instr->getSourceRef());
                m_ctx->getDiagCollector()->builder(Diag_Trace, "ExpandScalarAction")
                        << std::format("Allocated temporary high virtual register 't{}_hi'", idx).c_str()
                        << instr->getSourceRef();
            }
            return tempsHi[idx];
        }
        else
        {
            if (!tempsLo[idx])
            {
                tempsLo[idx] = opBuilder.buildVReg(halfType, std::format("t{}_lo", idx).c_str(), instr->getSourceRef());
                m_ctx->getDiagCollector()->builder(Diag_Trace, "ExpandScalarAction")
                        << std::format("Allocated temporary low virtual register 't{}_lo'", idx).c_str()
                        << instr->getSourceRef();
            }
            return tempsLo[idx];
        }
    };

    for (const auto &expInstr : recipe->m_expandSequence)
    {
        std::vector<MirOperand *> newOps;
        newOps.reserve(expInstr.m_operands.size());

        for (const auto &recipeOp : expInstr.m_operands)
        {
            MirOperand *resolvedOp = nullptr;
            switch (recipeOp.kind)
            {
                case ExpansionOperandKind::DestLow:
                    resolvedOp = destLo;
                    break;
                case ExpansionOperandKind::DestHigh:
                    resolvedOp = destHi;
                    break;
                case ExpansionOperandKind::Src0Low:
                    resolvedOp = srcLo;
                    break;
                case ExpansionOperandKind::Src0High:
                    resolvedOp = srcHi;
                    break;
                case ExpansionOperandKind::TemporalLow0:
                    resolvedOp = getTempRegister(0, false);
                    break;
                case ExpansionOperandKind::TemporalHigh0:
                    resolvedOp = getTempRegister(0, true);
                    break;
                case ExpansionOperandKind::TemporalLow1:
                    resolvedOp = getTempRegister(1, false);
                    break;
                case ExpansionOperandKind::TemporalHigh1:
                    resolvedOp = getTempRegister(1, true);
                    break;
                case ExpansionOperandKind::TemporalLow2:
                    resolvedOp = getTempRegister(2, false);
                    break;
                case ExpansionOperandKind::TemporalHigh2:
                    resolvedOp = getTempRegister(2, true);
                    break;
                case ExpansionOperandKind::IntImm:
                    resolvedOp = opBuilder.buildInt(halfType, recipeOp.intVal);
                    break;
                case ExpansionOperandKind::FloatImm:
                    resolvedOp = opBuilder.buildFloat(halfType, recipeOp.floatVal);
                    break;

                case ExpansionOperandKind::MemoryHalfOffset:
                {
                    MirOperand *baseOp = (recipeOp.memVal.m_baseKind == ExpansionOperandKind::Src0Low) ? srcLo : destLo;
                    int32_t factor = recipeOp.memVal.m_scaleHalfSizeFactor;

                    int64_t stride = factor * (halfType->getTotalSizeInBits() / 8);
                    int64_t finalOffset = stride + recipeOp.memVal.m_displ;

                    m_ctx->getDiagCollector()->builder(Diag_Trace, "ExpandScalarAction")
                            << std::format("Split mem operand '{}' into '{}'", baseOp->toString(), finalOffset).c_str()
                            << baseOp->getSourceRef();

                    if (baseOp && baseOp->isOfType<MirMemory>())
                    {
                        MirMemory *origMem = baseOp->get<MirMemory>();
                        FlexInt displ = origMem->getDisplacement()->getValue();

                        resolvedOp = opBuilder.buildMem(halfType,
                                                        origMem->getBase(),
                                                        FlexInt(finalOffset) + displ,
                                                        origMem->getSourceRef());
                    }
                    else
                    {
                        resolvedOp = opBuilder.buildMem(halfType,
                                                        baseOp->get<MirRegister>(),
                                                        FlexInt(finalOffset),
                                                        baseOp->getSourceRef());
                    }
                    break;
                }
                default:
                    break;
            }

            if (resolvedOp)
                newOps.push_back(resolvedOp);
        }

        if (!expInstr.m_rtLibraryCall.empty())
        {
            m_ctx->getDiagCollector()->builder(Diag_Trace, "ExpandScalarAction")
                    << std::format("Injecting RT call to '{}'", expInstr.m_rtLibraryCall).c_str()
                    << instr->getSourceRef();

            newOps.insert(newOps.begin(),
                          opBuilder.buildRtSymbol(expInstr.m_rtLibraryCall.data(), instr->getSourceRef()));
        }

        insertBeforeBuilder.build(expInstr.m_instr, instr->getSourceRef(), newOps);
    }

    // Clean up stream context by eliminating the un-expanded target node
    instrList.erase(it);
    return { .m_executed = true, .m_succeeded = true, .m_mirChanged = true };
}