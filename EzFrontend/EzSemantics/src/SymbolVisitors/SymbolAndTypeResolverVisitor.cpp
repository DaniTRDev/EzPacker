#include "SymbolVisitors/SymbolAndTypeResolverVisitor.h"

bool SymbolAndTypeResolverVisitor::visit(CodeScope *scope) { return AstNodeVisitor::visitAll(scope->getExpressions()); }

bool SymbolAndTypeResolverVisitor::visit(ConditionAstNode *cond)
{
    return cond->getLeft()->accept(this) && cond->getRight()->accept(this);
}

bool SymbolAndTypeResolverVisitor::visit(ForAstNode *_for)
{
    ScopeAnnotation *annotation = _for->getAnnotation<ScopeAnnotation>();
    ScopeGuard guard(getSemanticContext(), annotation->getOwnedScope());

    return _for->getInitialization()->accept(this) && _for->getCondition()->accept(this) &&
            _for->getNextItClause()->accept(this) && _for->getBody()->accept(this);
}

bool SymbolAndTypeResolverVisitor::visit(IfAstNode *ifNode)
{
    if (!ifNode->getCondition()->accept(this))
        return false;

    // True branch gets its own lexical scope
    {
        ScopeAnnotation *trueScope = ifNode->getTrueScope()->getAnnotation<ScopeAnnotation>();
        ScopeGuard trueGuard(getSemanticContext(), trueScope->getOwnedScope());
        if (!ifNode->getTrueScope()->accept(this))
            return false;
    }

    // False branch (if it exists) gets its own isolated lexical scope
    if (ifNode->getFalseScope())
    {
        ScopeAnnotation *falseScope = ifNode->getFalseScope()->getAnnotation<ScopeAnnotation>();
        ScopeGuard falseGuard(getSemanticContext(), falseScope->getOwnedScope());
        if (!ifNode->getFalseScope()->accept(this))
            return false;
    }

    return true;
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
                                            "SymbolAndTypeResolverVisitor::ImmediateOperand",
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
                                            "SymbolAndTypeResolverVisitor::ImmediateOperand",
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
    ScopeGuard guard(getSemanticContext(), annotation->getOwnedScope());

    return label->getCodeScope()->accept(this);
}

bool SymbolAndTypeResolverVisitor::visit(MemoryOperandAstNode *operand)
{
    const std::string_view &typeStr = operand->getReferencedMemoryDataTypeStr();
    std::shared_ptr<Type> type = nullptr;

    if (typeStr.empty())
    {
        type = TypeTable::getDefaultType();
    }
    else
    {
        type = TypeTable::getType(typeStr);
        if (!type)
        {
            getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                            "Unknown memory operand data type: " + std::string(typeStr),
                                            "SymbolAndTypeResolverVisitor::MemoryOperand",
                                            operand->getSourceRef());
            return false;
        }
    }

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

bool SymbolAndTypeResolverVisitor::visit(SwitchAstNode *_switch)
{
    return _switch->getSwitchVariable()->accept(this) && AstNodeVisitor::visitAll(_switch->getCases());
}

bool SymbolAndTypeResolverVisitor::visit(SwitchCaseAstNode *switchCase)
{
    ScopeAnnotation *annotation = switchCase->getAnnotation<ScopeAnnotation>();
    ScopeGuard guard(getSemanticContext(), annotation->getOwnedScope());
    return switchCase->getCaseValue()->accept(this) && switchCase->getBody()->accept(this);
}

bool SymbolAndTypeResolverVisitor::visit(WhileAstNode *whileNode)
{
    if (whileNode->getCondition() && !whileNode->getCondition()->accept(this))
    {
        return false;
    }

    // Loop body needs its own lexical scope
    ScopeAnnotation *annotation = whileNode->getAnnotation<ScopeAnnotation>();
    ScopeGuard guard(getSemanticContext(), annotation->getOwnedScope());
    return whileNode->getCodeScope()->accept(this);
}
