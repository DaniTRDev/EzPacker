#include "SymbolVisitors/SymbolAndTypeResolverVisitor.h"

bool SymbolAndTypeResolverVisitor::visit(CodeScope *scope) { return AstNodeVisitor::visitAll(scope->getExpressions()); }

bool SymbolAndTypeResolverVisitor::visit(ConditionAstNode *cond)
{
    return cond->getLeft()->accept(this) && cond->getRight()->accept(this);
}

bool SymbolAndTypeResolverVisitor::visit(IfAstNode *ifNode)
{
    return ifNode->getCondition()->accept(this) && ifNode->getTrueScope()->accept(this) &&
            (!ifNode->getFalseScope() || ifNode->getFalseScope()->accept(this));
}

bool SymbolAndTypeResolverVisitor::visit(ImmediateOperand *imm)
{
    const std::string_view &dataTypeStr = imm->getDataType();
    if (!dataTypeStr.empty())
    {
        // Annotate the type of this immediate.
        std::shared_ptr<Type> dataType = TypeTable::getType(dataTypeStr);
        if (!dataType)
        {
            getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                            "Invalid cast for immediate",
                                            "TypeCheckVisitor::ImmediateOperand",
                                            imm->getSourceRef());

            return false;
        }

        imm->createAnnotation<DataTypeAnnotation>(getSemanticContext()->getAnnotPool(), dataType.get());
    }
    else
    {
        std::shared_ptr<Type> dataType = imm->getImmediateType() == ImmediateType::Integer
                ? TypeTable::getType("i64")
                : TypeTable::getType("double");
        if (!dataType)
        {
            getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                            "Internal Compiler Error: default Immediate value not defined",
                                            "TypeCheckVisitor::ImmediateOperand",
                                            imm->getSourceRef());
            return false;
        }
        imm->createAnnotation<DataTypeAnnotation>(getSemanticContext()->getAnnotPool(), dataType.get());
    }
    return true;
}

bool SymbolAndTypeResolverVisitor::visit(Instruction *instr)
{
    if (instr->getInstructionName() == "create")
    {
        // We don't need to check if the symbol exists because the first pass MUST HAVE created it.
        return true;
    }

    return visitAll(instr->getExpressions());
}

bool SymbolAndTypeResolverVisitor::visit(Label *label)
{
    ScopedSymbolAnnotation *annotation = label->getAnnotation<ScopedSymbolAnnotation>();
    if (!annotation || !annotation->getOwnedScope())
    {
        getSemanticContext()->emitError(
                ErrorSeverity::Fatal,
                "Internal Compiler Error: Label has no associated scope (Definition pass failed?)",
                "SymbolAndTypeResolverVisitor::Label",
                label->getSourceRef());
        return false;
    }

    ScopeGuard guard(getSemanticContext(), annotation->getOwnedScope());
    return label->getCodeScope()->accept(this);
}

bool SymbolAndTypeResolverVisitor::visit(MemoryOperandAstNode *operand)
{
    const std::string_view &typeStr = operand->getReferencedMemoryDataTypeStr();
    std::shared_ptr<Type> type = TypeTable::getType(typeStr);

    if (!typeStr.empty() && !type)
    {
        getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                        "Internal Compiler Error: Given node type is not a valid type",
                                        "SymbolAndTypeResolverVisitor::MemoryOperandAstNode",
                                        operand->getSourceRef());
        return false;
    }

    type = TypeTable::getDefaultType();
    operand->createAnnotation<DataTypeAnnotation>(getSemanticContext()->getAnnotPool(), type.get());

    AstNode *base = nullptr, *index = nullptr;
    switch (operand->getMemoryOperandType())
    {
        case MemoryOperandType::BaseDisplacement:
        {
            base = dynamic_cast<BaseDisplacementMemory *>(operand)->getBase();
            return base->accept(this);
        }
        case MemoryOperandType::BaseIndexScaleDisplacement:
        {
            base = dynamic_cast<BaseIndexScaleDisplacementMemory *>(operand)->getBase();
            index = dynamic_cast<BaseIndexScaleDisplacementMemory *>(operand)->getIndex();
            return base->accept(this) && index->accept(this);
        }
        case MemoryOperandType::IndexScale:
        {
            index = dynamic_cast<IndexScaleMemory *>(operand)->getIndex();
            return index->accept(this);
        }
        default:
        {
            // If the memory operand does not have a base / scale (direct), just return true.
            return true;
        }
    }

    // This can't happen.
    return false;
}

bool SymbolAndTypeResolverVisitor::visit(Module *module)
{
    ScopedSymbolAnnotation *annotation = module->getAnnotation<ScopedSymbolAnnotation>();

    if (!annotation || !annotation->getOwnedScope())
    {
        getSemanticContext()->emitError(
                ErrorSeverity::Fatal,
                "Internal Compiler Error: Module has no associated scope (Definition pass failed?)",
                "SymbolAndTypeResolverVisitor::Module",
                module->getSourceRef());
        return false;
    }

    ScopeGuard guard(getSemanticContext(), annotation->getOwnedScope());
    return module->getBody()->accept(this);
}

bool SymbolAndTypeResolverVisitor::visit(Variable *var)
{
    if (var->getAnnotation<SymbolAnnotation>() != nullptr)
    {
        /*
         * If the variable already has a symbol annotation, it means this is a global variable. We don't need to check
         * if this symbol exists.
         */
        return true;
    }

    Symbol *symbol = nullptr;
    const std::string_view &variableName = var->getVariableName();

    if (!getSemanticContext()->resolveSymbolInScope(variableName, &symbol, true))
    {
        getSemanticContext()->emitUnknownSymbolError("SymbolAndTypeResolverVisitor::Variable", variableName, var);
        return false;
    }

    var->createAnnotation<SymbolAnnotation>(getSemanticContext()->getAnnotPool(), symbol);
    return true;
}

bool SymbolAndTypeResolverVisitor::visit(WhileAstNode *whileNode)
{
    return whileNode->getCondition()->accept(this) && whileNode->getCodeScope()->accept(this);
}
