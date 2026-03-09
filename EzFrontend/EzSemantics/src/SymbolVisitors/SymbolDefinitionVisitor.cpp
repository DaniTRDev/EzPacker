#include "SymbolVisitors/SymbolDefinitionVisitor.h"
// Assume ScopeCreatorGuard is included or available via context headers

bool DefineSymbolFromVariable(SymbolType symbolType,
                              const std::shared_ptr<BasicSemanticContext> &ctx,
                              AstNode *node,
                              const std::string &moduleName)
{
    if (node->getType() != AstNodeType::Variable)
    {
        ctx->emitError(ErrorSeverity::Fatal, "Expected variable for symbol", moduleName, node->getSourceRef());
        return false;
    }

    Symbol *symbol = nullptr;
    Variable *variable = (Variable *)node;
    const std::string_view &variableDataType = variable->getVariableDataType();
    const std::string_view &variableName = variable->getVariableName();

    std::shared_ptr<Type> dataType = TypeTable::getType(variableDataType);
    if (!dataType || dataType->getUnderlyingType() == UnderlyingType::Void)
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Invalid data type provided for variable",
                       moduleName,
                       variable->getSourceRef());
        return false;
    }

    Symbol *upperScopeSymbol = nullptr;
    bool isSymbolDefinedInParentScopes = ctx->resolveSymbolInScope(variableName, &upperScopeSymbol, true);

    if (!ctx->createSymbol(variable, symbolType, &symbol, dataType.get(), variableName))
    {
        ctx->emitSymbolRedefinitionError(moduleName, variableName, variable);
        return false;
    }

    if (isSymbolDefinedInParentScopes)
    {
        ctx->emitError(ErrorSeverity::Warning,
                       "Variable shadows another variable defined in upper scopes",
                       moduleName,
                       variable->getSourceRef());

        ctx->emitError(ErrorSeverity::Warning,
                       "Previously defined here",
                       moduleName,
                       upperScopeSymbol->getDefiningNode()->getSourceRef());
    }

    variable->createAnnotation<SymbolAnnotation>(ctx->getAnnotPool(), symbol);
    return true;
}

bool SymbolDefinitionVisitor::visit(struct CodeScope *scope)
{
    return AstNodeVisitor::visitAll(scope->getExpressions());
}

bool SymbolDefinitionVisitor::visit(struct ForAstNode *_for)
{
    // A 'for' loop creates an overarching scope so variables declared in the
    // initialization block are available to the condition, nextIt, and body.
    ScopeCreatorGuard guard(_for, getSemanticContext(), "ForLoopScope");

    return _for->getInitialization()->accept(this) && _for->getCondition()->accept(this) &&
            _for->getBody()->accept(this) &&
            // Ensure we safely handle nextIt since it was parsed separately
            (_for->getNextItClause() ? _for->getNextItClause()->accept(this) : true);
}

bool SymbolDefinitionVisitor::visit(IfAstNode *ifNode)
{
    if (!ifNode->getCondition()->accept(this))
        return false;

    // True branch gets its own lexical scope
    {
        ScopeCreatorGuard trueGuard(ifNode->getTrueScope(), getSemanticContext(), "IfTrueScope");
        if (!ifNode->getTrueScope()->accept(this))
            return false;
    }

    // False branch (if it exists) gets its own isolated lexical scope
    if (ifNode->getFalseScope())
    {
        ScopeCreatorGuard falseGuard(ifNode->getFalseScope(), getSemanticContext(), "IfFalseScope");
        if (!ifNode->getFalseScope()->accept(this))
            return false;
    }

    return true;
}

bool SymbolDefinitionVisitor::visit(Instruction *instr)
{
    if (instr->getInstructionName() != "create")
        return true;

    auto operands = instr->getExpressions();
    if (!operands || operands->m_numElems == 0)
    {
        getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                        "Create instruction must have at least 1 operand",
                                        "SymbolDefinitionVisitor::Instruction",
                                        instr->getSourceRef());
        return false;
    }
    else if (operands->m_numElems > 1)
    {
        getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                        "Create instruction can only have 1 operand",
                                        "SymbolDefinitionVisitor::Instruction",
                                        instr->getSourceRef());
        return false;
    }

    AstNode *operand = (AstNode *)operands->m_head->m_object;
    return DefineSymbolFromVariable(SymbolType::LocalVariable,
                                    getSemanticContext(),
                                    operand,
                                    "SymbolDefinitionVisitor::Instruction");
}

bool SymbolDefinitionVisitor::visit(Label *label)
{
    Symbol *symbol = nullptr;
    Scope *ownedScope = nullptr;
    const std::string_view &labelName = label->getLabelName();

    if (!getSemanticContext()->createSymbol(label, SymbolType::Label, &symbol, nullptr, labelName))
    {
        getSemanticContext()->emitSymbolRedefinitionError("SymbolDefinitionVisitor::Label", labelName, label);
        return false;
    }

    {
        ScopeCreatorGuard guard(label, getSemanticContext(), std::string(labelName));
        ownedScope = getSemanticContext()->getCurrentScope();

        if (!label->getCodeScope()->accept(this))
        {
            return false;
        }
    }

    ScopedSymbolAnnotation *annotation = label->createAnnotation<ScopedSymbolAnnotation>(m_ctx->getAnnotPool());
    annotation->setOwnedScope(ownedScope);
    annotation->setSymbol(symbol);

    return true;
}

bool SymbolDefinitionVisitor::visit(struct ModuleHeader *header)
{
    return AstNodeVisitor::visitAll(header->getExpressions());
}

bool SymbolDefinitionVisitor::visit(Module *module)
{
    CodeScope *body = module->getBody();
    ModuleHeader *header = module->getHeader();
    Symbol *moduleSymbol = nullptr;
    Scope *ownedScope = nullptr;

    const std::string_view &moduleName = header->getModuleName();
    const std::string_view &moduleReturn = header->getReturnTypeName();

    std::shared_ptr<Type> moduleReturnType = TypeTable::getType(moduleReturn);
    if (!moduleReturnType)
    {
        getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                        "Invalid return type provided for module",
                                        "SymbolDefinitionVisitor::Module",
                                        header->getSourceRef());
        return false;
    }

    if (!getSemanticContext()
                 ->createSymbol(module, SymbolType::Module, &moduleSymbol, moduleReturnType.get(), moduleName))
    {
        getSemanticContext()->emitSymbolRedefinitionError("SymbolDefinitionVisitor::Module", moduleName, module);
        return false;
    }

    {
        ScopeCreatorGuard guard(module, getSemanticContext(), std::string(moduleName));
        ownedScope = getSemanticContext()->getCurrentScope();

        if (!header->accept(this) || !body->accept(this))
        {
            return false;
        }
    }

    ScopedSymbolAnnotation *annotation = module->createAnnotation<ScopedSymbolAnnotation>(m_ctx->getAnnotPool());
    annotation->setOwnedScope(ownedScope);
    annotation->setSymbol(moduleSymbol);
    return true;
}

bool SymbolDefinitionVisitor::visit(Variable *variable)
{
    SymbolType type = SymbolType::LocalVariable;
    if (getSemanticContext()->isCurrentScopeGlobalScope())
    {
        type = SymbolType::GlobalVariable;
    }

    return DefineSymbolFromVariable(type, getSemanticContext(), variable, "SymbolDefinitionVisitor::Variable");
}

bool SymbolDefinitionVisitor::visit(SwitchAstNode *_switch)
{
    // The switch variable does not need a new scope, just traverse it
    // and then traverse all the cases.
    return AstNodeVisitor::visitAll(_switch->getCases());
}

bool SymbolDefinitionVisitor::visit(SwitchCaseAstNode *_switchCase)
{
    // Each case block requires its own scope to prevent variable leakage between cases
    ScopeCreatorGuard guard(_switchCase, getSemanticContext(), "SwitchCaseScope");
    return _switchCase->getBody()->accept(this);
}

bool SymbolDefinitionVisitor::visit(WhileAstNode *whileNode)
{
    if (whileNode->getCondition() && !whileNode->getCondition()->accept(this))
    {
        return false;
    }

    // Loop body needs its own lexical scope
    ScopeCreatorGuard guard(whileNode, getSemanticContext(), "WhileBodyScope");
    return whileNode->getCodeScope()->accept(this);
}