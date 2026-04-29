#include "Lowerers/VariableLowerer.h"

bool VariableLowerer::lower(AstNode *node, AstLoweringContext *ctx)
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
        ctx->pushOperand(MirReference{ .m_type = MirReferenceType::Function, .m_refId = ctx->getMirIdOfSymbol(sym) });
    }

    return true;
}

bool VariableLowerer::lowerGlobalVariable(Variable *var, AstLoweringContext *ctx)
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
        // Reserve the total size for every element (Zero-initialized).
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

    MirGlobalDataEntry *entry = ctx->getEmitterContext()->createGlobalData(tempBuffer.data(), tempBuffer.size(), false);
    return ctx->linkSymbolToMirId(sym, entry->m_entryId);
}

bool VariableLowerer::lowerLocalVariable(Variable *var, AstLoweringContext *ctx)
{
    SymbolAnnotation *symbolAnnot = var->getAnnotation<SymbolAnnotation>();
    Symbol *sym = symbolAnnot->getSymbol();
    size_t varSize = static_cast<size_t>(sym->getSymbolDataType()->getUnderlyingTypeSize());

    if (ctx->isSymbolLinkedToMir(sym))
    {
        if (var->getAnnotation<TypeCastAnnotation>())
        {
            // There's a cast in this variable usage, forward the lowering to the cast helper.
            return lowerVariableCast(var, ctx);
        }

        size_t existingVRegId = ctx->getMirIdOfSymbol(sym);
        ctx->pushOperand(MirOperand{ MirRegister{ .m_id = existingVRegId, .m_sizeInBytes = varSize / 8 } });

        return true;
    }

    // This piece of code will only be called when lowering a CREATE instruction.
    MirRegister vReg = ctx->getEmitter()->createVirtualRegister(varSize / 8);

    // Link it so future usages find it.
    ctx->linkSymbolToMirId(sym, vReg.m_id);
    ctx->pushOperand(MirOperand{ vReg });

    return true;
}

bool VariableLowerer::lowerVariableCast(Variable *var, AstLoweringContext *ctx)
{
    SymbolAnnotation *symbolAnnot = var->getAnnotation<SymbolAnnotation>();
    TypeCastAnnotation *castAnnot = var->getAnnotation<TypeCastAnnotation>();

    Symbol *sym = symbolAnnot->getSymbol();
    Type *destType = castAnnot->getDataType();

    size_t destTypeSize = static_cast<size_t>(destType->getUnderlyingTypeSize());
    size_t existingVRegId = ctx->getMirIdOfSymbol(sym);
    size_t sourceTypeSize = static_cast<size_t>(sym->getSymbolDataType()->getUnderlyingTypeSize());

    MirRegister resultVReg = ctx->getEmitter()->createVirtualRegister(destTypeSize / 8);
    MirRegister sourceVReg = MirRegister{ .m_id = existingVRegId, .m_sizeInBytes = sourceTypeSize };

    if (sourceTypeSize != destTypeSize)
    {
        if (castAnnot->isExpansion())
        {
            // Emit an XEXT instruction where X is Z (non-signed) or S (signed).
            if (destType->isSigned())
            {
                ctx->getEmitter()->emitSEXT(resultVReg, sourceVReg);
            }
            else
            {
                ctx->getEmitter()->emitZEXT(resultVReg, sourceVReg);
            }
        }
        else if (castAnnot->isTruncation())
        {
            // Emit a TRUNC instruction.
            ctx->getEmitter()->emitTRUNC(resultVReg, sourceVReg);
        }
        else
        {
            ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                                 "Invalid type cast annotation on variable usage",
                                                 "VariableLowerer");
            return false;
        }
    }

    // Now, push the result register as the operand for this variable usage.
    ctx->pushOperand(MirOperand{ resultVReg });
    return true;
}
