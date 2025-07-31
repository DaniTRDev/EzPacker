#include "NodeAnnotators/ModuleAnnotator.h"

ModuleAnnotator::ModuleAnnotator(const ScopeAbleAnnotator::SymbolTableT &symbolTable,
                                 const ScopeAbleAnnotator::TypeTableT &typeTable)
{
    setSymbolTable(symbolTable);
    setTypeTable(typeTable);
}

bool ModuleAnnotator::annotate(const std::shared_ptr<AstNode> &node, const std::shared_ptr<SourceLoggingSink> &logger)
{
    if (!logger)
        return false; // If no logger, exit.

    if (!node)
    {
        logger->logError(LogMessage("Invalid module node"));
        return false;
    }

    const ScopeAbleAnnotator::SymbolTableT &symbolTable = getSymbolTable();
    const ScopeAbleAnnotator::TypeTableT &typeTable = getTypeTable();
    std::shared_ptr<InstructionAnnotator> iiAnnotator = std::make_shared<InstructionAnnotator>(symbolTable, typeTable);
    std::shared_ptr<LabelAnnotator> llAnnotator = std::make_shared<LabelAnnotator>(symbolTable, typeTable);
    std::shared_ptr<ModuleAnnotation> annot = std::make_shared<ModuleAnnotation>(0, 0); // Set symbol and type to 0.
    const std::shared_ptr<SourceReference> &sourceRef = node->getSourceRef();

    if (!annotateHeader(node, annot, logger))
    {
        logger->logSourceError(LogMessage(""), sourceRef);
        return false;
    }

    node->removeChild(); // Remove header.

    // Module body.
    for (size_t i = 0; i < node->getChildren().size(); i++)
    {
        // Child will only be an instruction or a label (thanks to ParsingRule).
        const std::shared_ptr<AstNode> &child = node->getChild(i);
        if (child->getId() == AstNodes::Instruction().getId())
        {
            if (!iiAnnotator->annotate(child, logger))
            {
                logger->logSourceError(LogMessage("Invalid instruction"), child->getSourceRef());
                return false;
            }
        }
        else
        {
            if (!llAnnotator->annotate(child, logger))
            {
                logger->logSourceError(LogMessage("Invalid label"), child->getSourceRef());
                return false;
            }
        }
    }

    node->setAnnotation(annot);
    symbolTable->endScope();

    return true;
}

bool ModuleAnnotator::canAnnotate(const std::shared_ptr<AstNode> &node)
{
    return node->getId() == AstNodes::Module().getId();
}

bool ModuleAnnotator::annotateHeader(const std::shared_ptr<AstNode> &node,
                                     const std::shared_ptr<ModuleAnnotation> &annot,
                                     const std::shared_ptr<SourceLoggingSink> &logger)
{
    // First, annotate module's symbol.
    const ScopeAbleAnnotator::SymbolTableT &symbolTable = getSymbolTable();
    const ScopeAbleAnnotator::TypeTableT &typeTable = getTypeTable();
    const std::shared_ptr<SourceReference> &sourceRef = node->getSourceRef();
    const std::shared_ptr<AstNode> &moduleHeader = node->getChild(0);
    const std::shared_ptr<AstNode> &moduleName = moduleHeader->getChild(1);
    const std::shared_ptr<AstNode> &moduleType = moduleHeader->getChild(0);

    std::shared_ptr<TypeAnnotator> ttAnnotator = std::make_shared<TypeAnnotator>(typeTable);
    std::shared_ptr<SymbolAnnotator> ssAnnotator = std::make_shared<SymbolAnnotator>(symbolTable);
    std::shared_ptr<VirtualVariableAnnotator> vvAnnotator = std::make_shared<VirtualVariableAnnotator>(symbolTable);

    ssAnnotator->setWorkingMode(SymbolAnnotatorWorkingMode::CreateNewSymbol);
    if (!ttAnnotator->annotate(moduleType, logger))
    {
        logger->logSourceError(LogMessage("Invalid module return type {}", moduleType->getContent()),
                               moduleType->getSourceRef());
        return false;
    }
    if (!ssAnnotator->annotate(moduleName, logger))
    {
        logger->logSourceError(LogMessage("Invalid module name {}", moduleName->getChild(0)->getContent()),
                               moduleName->getSourceRef());
        return false;
    }

    moduleHeader->removeChild(); // Module Type
    moduleHeader->removeChild(); // Module Name

    annot->setTypeId(std::dynamic_pointer_cast<TypeAnnotation>(moduleType->getAnnotation())->getTypeId());
    annot->setSymbolId(std::dynamic_pointer_cast<SymbolAnnotation>(moduleName->getAnnotation())->getSymbolId());

    // Start with arguments.
    // Function arguments must be annotated into new symbols of the newly created module scope.
    symbolTable->beginScope();
    vvAnnotator->setWorkingMode(VVAnnotatorWorkingMode::CreateNewSymbol);
    for (size_t i = 0; i < moduleHeader->getChildren().size(); i += 2)
    {
        /*
         * We can assume that i is a type node and i+1 is a virtual variable node because the PARSER already did the
         * job. We suppose that the AST we were given is perfectly formed (types and symbol declarations might be wrong,
         * but the grammar structure is correct).
         */
        const std::shared_ptr<AstNode> &argumentTypeNode = moduleHeader->getChild(i);
        const std::shared_ptr<AstNode> &argumentContentNode = moduleHeader->getChild(i + 1);

        if (!ttAnnotator->annotate(argumentTypeNode, logger))
        {
            logger->logSourceError(LogMessage("Invalid argument type {}", argumentTypeNode->getContent()),
                                   argumentTypeNode->getSourceRef());
            return false;
        }

        vvAnnotator->setTypeId(
                std::dynamic_pointer_cast<TypeAnnotation>(argumentTypeNode->getAnnotation())->getTypeId());

        if (!vvAnnotator->annotate(argumentContentNode, logger))
        {
            logger->logSourceError(
                    LogMessage("Expected argument to be a virtual variable {}", argumentContentNode->getContent()),
                    argumentTypeNode->getSourceRef());
            return false;
        }

        /*
         * Now we have the type and symbol of the argument. Let's annotate it into our ModuleAnnotation. We can't remove
         * the child because we are iterating lineally, and our indexes would break, keep it annotated until we finish
         * annotating arguments, then we clear every child of the header.
         */
        annot->pushArgument(
                std::dynamic_pointer_cast<SymbolAnnotation>(argumentContentNode->getAnnotation())->getSymbolId());
    }

    moduleHeader->clear(); // Clear everything.
    return true;
}
