#ifndef EZPACKER_VALUENODE_H
#define EZPACKER_VALUENODE_H

#include "Ast.h"
#include "EzFrontendCommon.h"

enum class ValueNodeType : uint8_t
{
    Invalid = 0,
    Int,
    FloatingPoint, // FloatingPoint + double
    String
};

class ValueNode : public Ast
{
  public:
    /**
     * Creates the object with the given value type.
     * @param valueType
     */
    ValueNode(ValueNodeType valueType);
    
    /**
     * Returns the value type of this node.
     * @return NodeValueType
     */
    ValueNodeType getValueType();

    /**
     * Sets the value type of this node.
     * @param type
     */
    void setValueType(ValueNodeType type);

    /**
     * Clones the object. Must be overridden by base classes. Returns nullptr if there was an error.
     * @return std::shared_ptr<Ast>
     */
    std::shared_ptr<Ast> clone() const override;
    
  private:
    ValueNodeType m_valueType;
};

#endif // EZPACKER_VALUENODE_H
