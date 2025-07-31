#include "NodeAnnotators/TypeAnnotator.h"

TypeAnnotator::TypeAnnotator(const std::shared_ptr<ScopedTable<ScopedType>> &typeTable) { setTypeTable(typeTable); }

bool TypeAnnotator::annotate(const std::shared_ptr<AstNode> &node, const std::shared_ptr<SourceLoggingSink> &logger)
{
    if (!logger)
        return false; // If no logger, exit

    if (!node)
    {
        logger->logError(LogMessage("Invalid type node"));
        return false;
    }

    size_t typeId = 0;
    const std::shared_ptr<SourceReference> &sourceRef = node->getSourceRef();
    const std::string &typeName = node->getContent();

    if (!getTypeTable()->doesElemExistAtAnyUpperScopeByString(ScopedType::getStringFromName(typeName), typeId))
    {
        logger->logSourceError(LogMessage("Undefined type {}", typeName), sourceRef);
        return false;
    }

    node->setContent(""); // Safety measure.
    node->setAnnotation(std::make_shared<TypeAnnotation>(typeId));

    return true;
}

bool TypeAnnotator::canAnnotate(const std::shared_ptr<AstNode> &node)
{
    return node->getId() == AstNodes::Type().getId();
}
