#include "SymbolResolverVisitor/SymbolResolverVisitor.h"

bool SymbolResolverVisitor::visit(Instruction *instr)
{
    const std::shared_ptr<ErrorCollector> errorCollector = m_ctx->getErrorCollector();
    const std::shared_ptr<ScopeManager> scopeMgr = m_ctx->getScopeManager();

    for(auto &operand : instr->getOperands())
    {
        switch (operand->getType())
        {
            case AstNodeType::MemoryOperand:
            {
                return visit((class MemoryOperandAstNode*)operand.get());
            }
            case AstNodeType::Variable:
            {
                return visit((class Variable*)operand.get());
            }
            
            default:
                break;
        }
    }

    return true;
}

bool SymbolResolverVisitor::visit(Label *label)
{
    const std::shared_ptr<ErrorCollector> errorCollector = m_ctx->getErrorCollector();
    const std::shared_ptr<ScopeManager> scopeMgr = m_ctx->getScopeManager();
    std::shared_ptr<Symbol> symbol = nullptr;

    if (!scopeMgr->createSymbolAtCurrentScope(SymbolType::Label, label->getLabelName(), "", &symbol))
    {
        errorCollector->error(LogMessage("Symbol redefinition, current: label, previous: {}", symbol->getName()),
                              label->getSourceRef());
        return false;
    }

    scopeMgr->beginScope();
    {
        for (auto &expression : label->getExpressions())
        {
            AstNode *node = GetLabelExpressionAsNode(expression).get();
            if (node->getType() == AstNodeType::Label)
            {
                return visit((Label *)node);
            }
            return true;
        }
    }
    scopeMgr->endScope();

    label->setAnnotation(std::make_shared<SymbolAnnotation>(std::move(symbol)));
    return true;
}

bool SymbolResolverVisitor::visit(Module *module)
{
    ModuleBody *body = module->getBody().get();
    ModuleHeader *header = module->getHeader().get();
    const std::shared_ptr<ErrorCollector> errorCollector = m_ctx->getErrorCollector();
    const std::shared_ptr<ScopeManager> scopeMgr = m_ctx->getScopeManager();
    std::shared_ptr<Symbol> symbol = nullptr;

    if (!scopeMgr->createSymbolAtCurrentScope(SymbolType::Module,
                                              header->getModuleName(),
                                              header->getReturnType(),
                                              &symbol))
    {
        errorCollector->error(LogMessage("Symbol redefinition, current: module, previous: {}", symbol->getName()),
                              header->getSourceRef());
        return false;
    }

    scopeMgr->beginScope();
    {
        for (auto &param : header->getParameters())
        {
            Variable *variable = (Variable *)param.get();

            if (!visit(variable))
            {
                errorCollector->error(LogMessage("Invalid module parameter symbol: {}", symbol->getName()),
                                      variable->getSourceRef());

                return false;
            }
        }
    }
    scopeMgr->endScope();
    header->setAnnotation(std::make_shared<SymbolAnnotation>(std::move(symbol)));

    // Body
    scopeMgr->beginScope();
    {
        for (auto &expression : body->getExpressions())
        {
            AstNode *node = GetLabelExpressionAsNode(expression).get();
            if (node->getType() == AstNodeType::Label)
            {
                return visit((Label *)node);
            }
            return true;
        }
    }
    scopeMgr->endScope();

    return true;
}

bool SymbolResolverVisitor::visit(Variable *var)
{
    const std::shared_ptr<ErrorCollector> errorCollector = m_ctx->getErrorCollector();
    const std::shared_ptr<ScopeManager> scopeMgr = m_ctx->getScopeManager();

    SymbolType symbolType = scopeMgr->isTopScope() ? SymbolType::GlobalVariable : SymbolType::LocalVariable;
    std::shared_ptr<Symbol> symbol;

    if (!scopeMgr->createSymbolAtCurrentScope(symbolType, var->getVariableName(), var->getVariableDataType(), &symbol))
    {
        errorCollector->error(
                LogMessage("Symbol redefinition, current GlobalVariable, previous: {}", symbol->getName()),
                var->getSourceRef());
        return false;
    }

    var->setAnnotation(std::make_shared<SymbolAnnotation>(std::move(symbol)));
    return true;
}
