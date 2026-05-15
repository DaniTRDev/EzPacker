#include "AstNode/AstNodeVisitor.h"

bool AstNodeVisitor::visitAll(TypedPoolLinkedList<AstNode> *nodeList)
{
    if (!nodeList || nodeList->m_numElems == 0)
        return true;

    for (AstNode *node : *nodeList)
    {
        if (!node->accept(this))
        {
            return false;
        }
    }
    return true;
}
