#include "SymbolVisitors/TypeCheckVisitor.h"

bool TypeCheckVisitor::visit(const std::shared_ptr<struct CodeScope> &scope)
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

bool TypeCheckVisitor::visit(const std::shared_ptr<struct Instruction> &instr)
{
    for (auto &operand : instr->getOperands())
    {
        if (operand->getType() == AstNodeType::Immediate)
        {
            // Check if the type is an immediate and if it uses a data type.
            const std::string &dataTypeStr = std::dynamic_pointer_cast<ImmediateOperand>(operand)->getDataType();
            if (!dataTypeStr.empty())
            {
                // Annotate the type of this immediate.
                std::shared_ptr<Type> dataType = TypeTable::getType(dataTypeStr);
                if (!dataType)
                {
                    getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                                    "Invalid cast for immediate",
                                                    "TypeCheckVisitor::Instruction",
                                                    operand->getFirstSourceReference());

                    return false;
                }

                operand->addAnnotation(std::make_shared<DataTypeAnnotation>(dataType));
            }
            // TODO: Infer type and check if cast can be performed.
        }
        else if (operand->getType() == AstNodeType::MemoryOperand)
        {
            const std::string &dataTypeStr =
                    std::dynamic_pointer_cast<MemoryOperandAstNode>(operand)->getReferencedMemoryDataTypeStr();
            if (dataTypeStr.empty())
            {
                getSemanticContext()->emitError(ErrorSeverity::Warning,
                                                "Unknown memory operand data-type, using default i64",
                                                "TypeCheckVisitor::Instruction",
                                                instr->getFirstSourceReference());
                operand->addAnnotation(std::make_shared<DataTypeAnnotation>(TypeTable::getType("i64")));
            }
        }

        if (!visitBaseClass(operand))
        {
            // The concrete error of the fail will already be in the error collector.
            return false;
        }
    }

    return true;
}

bool TypeCheckVisitor::visit(const std::shared_ptr<struct Label> &label) { return visit(label->getCodeScope()); }

bool TypeCheckVisitor::visit(const std::shared_ptr<struct MemoryOperandAstNode> &operand)
{
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
            // If the memory operand does not have a base / scale (the case of direct), just return true.
            return true;
        }
    }

    // This can't happen.
    return false;
}

bool TypeCheckVisitor::visit(const std::shared_ptr<struct Module> &module) { return visit(module->getBody()); }

bool TypeCheckVisitor::visit(const std::shared_ptr<struct Variable> &var)
{
    std::shared_ptr<SymbolAnnotation> annotation = var->getAnnotation<SymbolAnnotation>();
    if (!annotation)
    {
        getSemanticContext()->emitError(
                ErrorSeverity::Fatal,
                "Internal Compiler Error: Variable has no associated symbol, Definition pass failed?",
                "TypeCheckVisitor::Variable",
                var->getFirstSourceReference());
        return false;
    }

    const std::shared_ptr<Symbol> &symbol = annotation->getSymbol();

    std::shared_ptr<Type> usedType = TypeTable::getType(var->getVariableDataType());
    if (!usedType)
    {
        // Variable didn't have attached a type, it will use symbol's type.
        return true;
    }

    std::shared_ptr<Type> symbolDataType = symbol->getSymbolDataType();
    if (usedType->getUnderlyingType() == UnderlyingType::Pointer ||
        usedType->getTypeName() == symbolDataType->getTypeName())
    {
        // Used type matches the type of the symbol, no cast needed.
        // No cast is neither needed for this type of symbol.
        return true;
    }

    // Check if casting is safe.
    if (usedType->getUnderlyingType() == symbolDataType->getUnderlyingType())
    {
        if (usedType->getUnderlyingTypeSize() > symbolDataType->getUnderlyingTypeSize())
        {
            getSemanticContext()->emitError(
                    ErrorSeverity::NoError,
                    "Used type is bigger than the original symbol size, this might result in more "
                    "instructions in the final code to expand the value",
                    "TypeCheckVisitor::Variable",
                    var->getFirstSourceReference());
        }
        else if (usedType->getUnderlyingTypeSize() < symbolDataType->getUnderlyingTypeSize())
        {
            getSemanticContext()->emitError(
                    ErrorSeverity::Warning,
                    "Used type is smaller than the original symbol size, this might result in a data loss",
                    "TypeCheckVisitor::Variable",
                    var->getFirstSourceReference());
        }
    }
    else
    {
        if (symbolDataType->getUnderlyingType() == UnderlyingType::String)
        {
            getSemanticContext()->emitError(
                    ErrorSeverity::Fatal,
                    std::format("Can't perform a cast from string to '{}'", usedType->getTypeName()),
                    "TypeCheckVisitor::Variable",
                    var->getFirstSourceReference());
            return false;
        }

        if (usedType->getUnderlyingType() == UnderlyingType::String)
        {
            getSemanticContext()->emitError(
                    ErrorSeverity::Fatal,
                    std::format("Can't perform a cast from '{}' to string", symbolDataType->getTypeName()),
                    "TypeCheckVisitor::Variable",
                    var->getFirstSourceReference());
            return false;
        }

        // Types are different, we need to be cautious.
        getSemanticContext()->emitError(ErrorSeverity::Warning,
                                        std::format("Explicit cast from '{}' to '{}' might cause a data loss",
                                                    symbolDataType->getTypeName(),
                                                    usedType->getTypeName()),
                                        "TypeCheckVisitor::Variable",
                                        var->getFirstSourceReference());
    }

    var->addAnnotation(std::make_shared<TypeCastAnnotation>(symbol, usedType));
    return true;
}
