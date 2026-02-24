#include "SymbolVisitors/SymbolAndTypeResolverVisitor.h"

bool SymbolAndTypeResolverVisitor::visit(const std::shared_ptr<struct CodeScope> &scope)
{
    for (auto &[id, expression] : scope->getExpressions())
    {
        if (!visitBaseClass(expression))
        {
            // The concrete error of the fail will already be in the error collector.
            return false;
        }
    }

    return true;
}

bool SymbolAndTypeResolverVisitor::visit(const std::shared_ptr<struct Instruction> &instr)
{
    if (instr->getInstructionName() == "create")
    {
        // We don't need to check if the symbol exists because the first pass MUST HAVE created it.
        return true;
    }

    for (auto &operand : instr->getOperands())
    {
        if (!visitBaseClass(operand))
        {
            // The concrete error of the fail will already be in the error collector.
            return false;
        }
    }

    return true;
}

bool SymbolAndTypeResolverVisitor::visit(const std::shared_ptr<struct Label> &label)
{
    std::shared_ptr<ScopedSymbolAnnotation> annotation = label->getAnnotation<ScopedSymbolAnnotation>();
    if (!annotation || !annotation->getOwnedScope())
    {
        getSemanticContext()->emitError(
                ErrorSeverity::Fatal,
                "Internal Compiler Error: Label has no associated scope (Definition pass failed?)",
                "SymbolAndTypeResolverVisitor::Label",
                label->getFirstSourceReference());
        return false;
    }

    ScopeGuard guard(getSemanticContext(), annotation->getOwnedScope());
    return visitBaseClass(label->getCodeScope());
}

bool SymbolAndTypeResolverVisitor::visit(const std::shared_ptr<struct MemoryOperandAstNode> &operand)
{
    const std::string &typeStr = operand->getReferencedMemoryDataTypeStr();
    std::shared_ptr<Type> type = TypeTable::getType(typeStr.empty() ? "pointer" : typeStr);

    operand->addAnnotation(std::make_shared<DataTypeAnnotation>(type));

    if (!type)
    {
        getSemanticContext()->emitError(
                ErrorSeverity::Fatal,
                "Internal Compiler Error: Given node type is not a valid type (not even a pointer)",
                "SymbolAndTypeResolverVisitor::MemoryOperandAstNode",
                operand->getFirstSourceReference());
        return false;
    }

    std::shared_ptr<AstNode> base, index;
    switch (operand->getMemoryOperandType())
    {
        case MemoryOperandType::BaseDisplacement:
        {
            base = std::dynamic_pointer_cast<BaseDisplacementMemory>(operand)->getBase();
            return visitBaseClass(base);
        }
        case MemoryOperandType::BaseIndexScaleDisplacement:
        {
            base = std::dynamic_pointer_cast<BaseIndexScaleDisplacementMemory>(operand)->getBase();
            index = std::dynamic_pointer_cast<BaseIndexScaleDisplacementMemory>(operand)->getIndex();
            return visitBaseClass(base) && visitBaseClass(index);
        }
        case MemoryOperandType::IndexScale:
        {
            index = std::dynamic_pointer_cast<IndexScaleMemory>(operand)->getIndex();
            return visitBaseClass(index);
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

bool SymbolAndTypeResolverVisitor::visit(const std::shared_ptr<struct Module> &module)
{
    const std::shared_ptr<CodeScope> &body = module->getBody();
    std::shared_ptr<ScopedSymbolAnnotation> annotation = module->getAnnotation<ScopedSymbolAnnotation>();

    if (!annotation || !annotation->getOwnedScope())
    {
        getSemanticContext()->emitError(
                ErrorSeverity::Fatal,
                "Internal Compiler Error: Module has no associated scope (Definition pass failed?)",
                "SymbolAndTypeResolverVisitor::Module",
                module->getFirstSourceReference());
        return false;
    }

    ScopeGuard guard(getSemanticContext(), annotation->getOwnedScope());
    return visit(body);
}

bool SymbolAndTypeResolverVisitor::visit(const std::shared_ptr<struct Variable> &var)
{
    if (!var->getAnnotations().empty())
    {
        /*
         * If the variable already has an annotation, it means this is a global variable. We don't need to check if
         * this symbol exists.
         */
        return true;
    }

    const std::string &variableName = var->getVariableName();
    std::shared_ptr<Symbol> symbol;

    if (!getSemanticContext()->resolveSymbolInScope(variableName, &symbol, true))
    {
        getSemanticContext()->emitUnknownSymbolError("SymbolAndTypeResolverVisitor::Variable", variableName, var);
        return false;
    }

    if (symbol->getSymbolDataType() && symbol->getSymbolDataType()->getUnderlyingType() == UnderlyingType::String)
    {
        getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                        "String type can't be used in local scopes",
                                        "SymbolAndTypeResolverVisitor::Variable",
                                        var->getFirstSourceReference());
        return false;
    }

    var->addAnnotation(std::make_shared<SymbolAnnotation>(symbol));
    return true;
}
