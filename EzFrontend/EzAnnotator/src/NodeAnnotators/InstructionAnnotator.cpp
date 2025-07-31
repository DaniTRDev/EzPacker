#include "NodeAnnotators/InstructionAnnotator.h"

InstructionAnnotator::InstructionAnnotator(const ScopeAbleAnnotator::SymbolTableT &symbolTable,
                                           const ScopeAbleAnnotator::TypeTableT &typeTable)
{
    setSymbolTable(symbolTable);
    setTypeTable(typeTable);
}

bool InstructionAnnotator::annotate(const std::shared_ptr<AstNode> &node,
                                    const std::shared_ptr<SourceLoggingSink> &logger)
{
    if (!logger)
        return false; // If we can't log, exit.

    if (!node)
    {
        logger->logError(LogMessage("Invalid instruction node"));
        return false;
    }

    const std::shared_ptr<SourceReference> &sourceRef = node->getSourceRef();
    const std::string &instructionName = node->getChild(0)->getContent();

    if (!g_InstructionTable.contains(instructionName))
    {
        logger->logSourceError(LogMessage("Invalid instruction name {}", instructionName), sourceRef);
        return false;
    }

    size_t instrId = g_InstructionTable.at(instructionName);

    node->removeChild(0); // Remove instruction name node.
    // TODO: Use Backend's instruction table. This is provisional.
    node->setAnnotation(std::make_shared<InstructionAnnotation>(instrId));

    // Annotate instruction operands.
    std::shared_ptr<ConstantAnnotator> ccAnnotator = std::make_shared<ConstantAnnotator>();
    std::shared_ptr<MemoryRefAnnotator> mmAnnotator = std::make_shared<MemoryRefAnnotator>(getSymbolTable());
    std::shared_ptr<TypeAnnotator> ttAnnotator = std::make_shared<TypeAnnotator>(getTypeTable());
    auto vvAnnotator = std::make_shared<VirtualVariableAnnotator>(getSymbolTable());

    for (size_t i = 0; i < node->getChildren().size(); i++)
    {
        std::shared_ptr<AstNode> operand = node->getChild(i);
        const std::shared_ptr<AstNode> &typeNode = operand->getChild(0);
        const std::shared_ptr<AstNode> &operandContent = operand->getChild(1);
        vvAnnotator->setWorkingMode(VVAnnotatorWorkingMode::ExpectsExistingSymbol);

        if (typeNode->getId() != AstNodes::Type().getId())
        {
            logger->logSourceError(LogMessage("Instruction operand must be preceded by a type"),
                                   typeNode->getSourceRef());
            return false;
        }
        if (!ttAnnotator->annotate(typeNode, logger))
        {
            logger->logSourceError(LogMessage("Invalid operand type"), typeNode->getSourceRef());
            return false;
        }

        size_t typeId = std::dynamic_pointer_cast<TypeAnnotation>(typeNode->getAnnotation())->getTypeId();

        if (operandContent->getId() == AstNodes::VirtualVariable().getId())
        {
            // Only LOAD instruction can create new virtual variables, which are defined in the FIRST operand.
            if (i == 0 && instrId == g_InstructionTable.at("load"))
                vvAnnotator->setWorkingMode(VVAnnotatorWorkingMode::CreateNewSymbol);

            vvAnnotator->setTypeId(typeId);
            if (!vvAnnotator->annotate(operandContent, logger))
            {
                logger->logSourceError(LogMessage("Invalid virtual variable operand"), operandContent->getSourceRef());
                return false;
            }
        }
        else if (operandContent->getId() == AstNodes::IntNumber().getId() ||
                 operandContent->getId() == AstNodes::FloatNumber().getId())
        {
            ccAnnotator->setTypeId(typeId);
            if (!ccAnnotator->annotate(operandContent, logger))
            {
                logger->logSourceError(LogMessage("Invalid constant integer or float operand"),
                                       operandContent->getSourceRef());
                return false;
            }
        }
        else if (operandContent->getId() == AstNodes::String().getId())
        {
            ccAnnotator->setTypeId(typeId);
            if (!ccAnnotator->annotate(operandContent, logger))
            {
                logger->logSourceError(LogMessage("Invalid constant string"), operandContent->getSourceRef());
                return false;
            }
        }
        else
        {
            // Memory Operand.
            mmAnnotator->setMemoryType(typeId);
            if (!mmAnnotator->annotate(operandContent, logger))
            {
                logger->logSourceError(LogMessage("Invalid memory reference operand"), operandContent->getSourceRef());
                return false;
            }
        }

        operand->setAnnotation(operandContent->getAnnotation()); // Move the annotation to the upper level.
        operand->removeChild();
        operand->removeChild();
    }

    return true;
}

bool InstructionAnnotator::canAnnotate(const std::shared_ptr<AstNode> &node)
{
    return node->getId() == AstNodes::Instruction().getId();
}
