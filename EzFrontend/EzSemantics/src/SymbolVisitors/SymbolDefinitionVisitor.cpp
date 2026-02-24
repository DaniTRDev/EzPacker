#include "SymbolVisitors/SymbolDefinitionVisitor.h"

bool DefineSymbolFromVariable(SymbolType symbolType,
                              const std::shared_ptr<BasicSemanticContext> &ctx,
                              const std::shared_ptr<AstNode> &node,
                              const std::string &moduleName)
{
    if (node->getType() != AstNodeType::Variable)
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Expected variable for symbol",
                       moduleName,
                       node->getFirstSourceReference());
        return false;
    }

    std::shared_ptr<Symbol> symbol;
    const std::shared_ptr<Variable> &variable = std::dynamic_pointer_cast<Variable>(node);
    const std::string &variableDataType = variable->getVariableDataType();
    const std::string &variableName = variable->getVariableName();

    std::shared_ptr<Type> dataType = TypeTable::getType(variableDataType);
    if (!dataType || dataType->getUnderlyingType() == UnderlyingType::Void)
    {
        ctx->emitError(ErrorSeverity::Fatal,
                       "Invalid data type provided for variable",
                       moduleName,
                       variable->getFirstSourceReference());
        return false;
    }

    std::shared_ptr<Symbol> upperScopeSymbol;
    bool isSymbolDefinedInParentScopes = ctx->resolveSymbolInScope(variableName, &upperScopeSymbol, true);

    if (!ctx->createSymbol(symbolType, variable, &symbol, dataType, variableName))
    {
        ctx->emitSymbolRedefinitionError(moduleName, variableName, variable);
        return false;
    }

    if (isSymbolDefinedInParentScopes)
    {
        ctx->emitError(ErrorSeverity::Warning,
                       "Variable shadows another variable defined in upper scopes",
                       moduleName,
                       variable->getFirstSourceReference());

        ctx->emitError(ErrorSeverity::Warning,
                       "Previously defined here",
                       moduleName,
                       upperScopeSymbol->getDefiningNode()->getFirstSourceReference());
    }

    variable->addAnnotation(std::make_shared<SymbolAnnotation>(symbol));
    return true;
}

bool SymbolDefinitionVisitor::visit(const std::shared_ptr<Instruction> &instr)
{
    if (instr->getInstructionName() != "create")
        return true;

    auto &operands = instr->getOperands();
    if (operands.empty())
    {
        getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                        "Create instruction must have at least 1 operand",
                                        "SymbolDefinitionVisitor::Instruction",
                                        instr->getFirstSourceReference());
        return false;
    }
    else if (operands.size() > 1)
    {
        getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                        "Create instruction can only have 1 operand",
                                        "SymbolDefinitionVisitor::Instruction",
                                        instr->getFirstSourceReference());
        return false;
    }

    const std::shared_ptr<AstNode> &operand = operands[0];
    return DefineSymbolFromVariable(SymbolType::LocalVariable,
                                    getSemanticContext(),
                                    operand,
                                    "SymbolDefinitionVisitor::Instruction");
}

bool SymbolDefinitionVisitor::visit(const std::shared_ptr<Label> &label)
{
    std::shared_ptr<Scope> ownedScope;
    std::shared_ptr<Symbol> symbol;
    const std::string &labelName = label->getLabelName();

    if (!getSemanticContext()->createSymbol(SymbolType::Label, label, &symbol, nullptr, labelName))
    {
        getSemanticContext()->emitSymbolRedefinitionError("SymbolDefinitionVisitor::Label", labelName, label);
        return false;
    }

    getSemanticContext()->beginScope(labelName);
    {
        ownedScope = getSemanticContext()->getCurrentScope();

        if (!AstNodeVisitor::visit(label))
        {
            return false;
        }
    }
    getSemanticContext()->endScope();

    std::shared_ptr<ScopedSymbolAnnotation> annotation = std::make_shared<ScopedSymbolAnnotation>();
    annotation->setOwnedScope(ownedScope);
    annotation->setSymbol(symbol);

    label->addAnnotation(annotation);
    return true;
}

bool SymbolDefinitionVisitor::visit(const std::shared_ptr<Module> &module)
{
    std::shared_ptr<Scope> ownedScope;
    std::shared_ptr<Symbol> moduleSymbol;
    const std::shared_ptr<ModuleHeader> &header = module->getHeader();
    const std::shared_ptr<CodeScope> &body = module->getBody();

    const std::string &moduleName = header->getModuleName();
    const std::string &moduleReturn = header->getReturnTypeName();

    std::shared_ptr<Type> moduleReturnType = TypeTable::getType(moduleReturn);
    if (!moduleReturnType)
    {
        getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                        "Invalid return type provided for module",
                                        "SymbolDefinitionVisitor::Module",
                                        header->getFirstSourceReference());
        return false;
    }

    if (!getSemanticContext()->createSymbol(SymbolType::Module, module, &moduleSymbol, moduleReturnType, moduleName))
    {
        getSemanticContext()->emitSymbolRedefinitionError("SymbolDefinitionVisitor::Module", moduleName, module);
        return false;
    }

    getSemanticContext()->beginScope(moduleName);
    {
        ownedScope = getSemanticContext()->getCurrentScope();

        // Header
        for (auto &param : header->getParameters())
        {
            if (!DefineSymbolFromVariable(SymbolType::ModuleParameter,
                                          getSemanticContext(),
                                          param,
                                          "SymbolDefinitionVisitor::Module"))
            {
                return false;
            }
        }

        // Body
        if (!AstNodeVisitor::visit(body))
        {
            // The concrete error of the fail will already be in the error collector.
            return false;
        }
    }
    getSemanticContext()->endScope();

    std::shared_ptr<ScopedSymbolAnnotation> annotation = std::make_shared<ScopedSymbolAnnotation>();
    annotation->setOwnedScope(ownedScope);
    annotation->setSymbol(moduleSymbol);

    module->addAnnotation(annotation);
    return true;
}

bool SymbolDefinitionVisitor::visit(const std::shared_ptr<struct Variable> &variable)
{
    if (!getSemanticContext()->isCurrentScopeGlobalScope())
    {
        throw std::runtime_error(
                "Internal compiler error: Called SymbolDefinitionVisitor::Variable on non-global variable.");
    }

    return DefineSymbolFromVariable(SymbolType::GlobalVariable,
                                    getSemanticContext(),
                                    variable,
                                    "SymbolDefinitionVisitor::Variable");
}
