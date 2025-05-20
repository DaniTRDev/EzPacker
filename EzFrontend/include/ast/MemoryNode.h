#ifndef EZPACKER_MEMORYNODE_H
#define EZPACKER_MEMORYNODE_H

#include "Ast.h"
#include "EzFrontendCommon.h"

class MemoryNode : public Ast
{
  public:
    /**
     * Creates the object and sets current type to given.
     * @param type
     */
    MemoryNode(IRMemoryReferenceType type);

    /**
     * Returns the reference type of this node.
     * @return IRMemoryReferenceType
     */
    IRMemoryReferenceType getMemRefType();

    /**
     * Sets the memory reference type.
     * @param type
     */
    void setMemRefType(IRMemoryReferenceType type);

    /**
     * Clones the object. Must be overridden by base classes. Returns nullptr if there was an error.
     * @return std::shared_ptr<Ast>
     */
    std::shared_ptr<Ast> clone() const override;
    
  private:
    IRMemoryReferenceType m_refType;
};

#endif // EZPACKER_MEMORYNODE_H
