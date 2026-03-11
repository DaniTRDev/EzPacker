#include "AstLowererVisitor/VariableLowerer.h"

bool VariableLowerer::lower(AstNode *node, LoweringContext *ctx)
{
    Variable *var = dynamic_cast<Variable *>(node);
    SymbolAnnotation *symbolAnnot = var->getAnnotation<SymbolAnnotation>();
    Symbol *sym = symbolAnnot->getSymbol();

    if (sym->getType() == SymbolType::LocalVariable)
    {
        // Local variable.
        return lowerLocalVariable(var, ctx);
    }
    else if (sym->getType() == SymbolType::GlobalVariable)
    {
        // Global variable.
        return lowerGlobalVariable(var, ctx);
    }
    else
    {
        // Module or label, emit a reference.
        ctx->pushOperand(MirReference{ .m_refId = ctx->getMirIdOfSymbol(sym) });
    }

    return true;
}

bool VariableLowerer::lowerGlobalVariable(Variable *var, LoweringContext *ctx)
{
    if (!ctx->getSemanticContext()->isCurrentScopeGlobalScope())
    {
        // Not in global scope, so this is a reference to a global variable within a code scope.
        Symbol *sym = var->getAnnotation<SymbolAnnotation>()->getSymbol();
        ctx->pushOperand(MirReference{ .m_refId = ctx->getMirIdOfSymbol(sym) });

        return true;
    }

    SymbolAnnotation *symbolAnnot = var->getAnnotation<SymbolAnnotation>();
    Symbol *sym = symbolAnnot->getSymbol();
    Type *symDataType = sym->getSymbolDataType();

    size_t varSize = static_cast<size_t>(symDataType->getUnderlyingType());
    size_t currentAddress = 0;
    std::vector<uint8_t> tempBuffer;

    TypedPoolSlice<AstNode> *initializers = var->getExpressions();

    if (varSize != 0)
    {
        // Reserve the total size for every element (Zero-initialized)
        tempBuffer.resize(initializers->m_numElems * varSize, 0);
    }

    for (size_t i = 0; i < initializers->m_numElems; i++)
    {
        switch (symDataType->getUnderlyingType())
        {
            case UnderlyingType::FloatingPoint:
            {
                FloatImmediate *_float = initializers->get<FloatImmediate>(i);
                double value = _float->getFloatingValue();

                std::memcpy(tempBuffer.data() + currentAddress, &value, varSize);
                currentAddress += varSize;

                break;
            }
            case UnderlyingType::Integer:
            {
                IntegerImmediate *integer = initializers->get<IntegerImmediate>(i);
                std::string bin = integer->getAsBin();

                size_t copySize = std::min(bin.size(), varSize);
                std::memcpy(tempBuffer.data() + currentAddress, bin.data(), copySize);

                currentAddress += varSize;
                break;
            }
            case UnderlyingType::String:
            {
                StringImmediate *strImm = initializers->get<StringImmediate>(i);
                const std::string_view &str = strImm->getStr();

                size_t strLenWithNull = str.size() + 1;
                tempBuffer.resize(currentAddress + strLenWithNull, 0);

                std::memcpy(tempBuffer.data() + currentAddress, str.data(), str.size());
                currentAddress += strLenWithNull;

                break;
            }
            case UnderlyingType::Void:
            case UnderlyingType::Invalid:
            default:
            {
                throw std::runtime_error("Internal Compiler Error: Invalid underlying data type for symbol");
            }
        }
    }

    MirGlobalDataEntry *entry =
            ctx->getGlobalDataEmitter()->createGlobalData(tempBuffer.data(), tempBuffer.size(), false);

    return ctx->linkSymbolToMirId(sym, entry->m_entryId);
}

bool VariableLowerer::lowerLocalVariable(Variable *var, LoweringContext *ctx)
{
    SymbolAnnotation *symbolAnnot = var->getAnnotation<SymbolAnnotation>();
    Symbol *sym = symbolAnnot->getSymbol();
    size_t varSize = static_cast<size_t>(sym->getSymbolDataType()->getUnderlyingTypeSize());

    if (ctx->isSymbolLinkedToMir(sym))
    {
        /*
         * We already have a vreg for this local variable, so we just reuse it. We don't need to care for the size not
         * matching because we have run a TypeChecker pass that should have already emitted a TypeCastAnnotation
         * that our parent caller should have seen and handled. This method should receive both the cast operand
         * and the original operand in the inserted cast instruction (TRUNC, ZEXT, SEXT, ...), so the size should
         * already be correct. If there is a mismatch, it's a bug in the TypeChecker pass, not here.
         */
        size_t existingVreg = ctx->getMirIdOfSymbol(sym);
        ctx->pushOperand(MirOperand{ MirRegister{ .m_id = existingVreg, .m_size = varSize / 8 } });

        return true;
    }

    MirRegister vReg = ctx->getEmitter()->createRegister(varSize / 8);

    // Link it so future usages find it
    ctx->linkSymbolToMirId(sym, vReg.m_id);
    ctx->pushOperand(MirOperand{ vReg });

    return true;
}
