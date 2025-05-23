#ifndef EZPACKER_AST_H
#define EZPACKER_AST_H

#include "EzFrontendCommon.h"
#include "IRTypes.h"

enum class AstType : uint8_t
{
    Invalid = 0,
    Identifier,
    Instruction,
    Memory, // Memory Reference Declaration.
    Module, // Module Declaration.
    Type,   // Type (IRType).
    TokenTypeNode,
    Value,          // Operand of type Number (int / float / string).
    Variable,       // Variable Declaration (global).
    VirtualVariable // Operand of type virtual variable.
};

class Ast
{
  public:
    /**
     * Destroys the object and releases resources.
     */
    virtual ~Ast();

    /**
     * Returns the type of this ast node.
     * @return AstType
     */
    AstType getType() const;

    /**
     * Returns true if this node has children nodes.
     * @return bool
     */
    bool hasChildren() const;

    /**
     * Adds a child to the current AST expression.
     * @param node
     */
    void addChild(std::shared_ptr<Ast> node);

    /**
     * Clears the vector of children.
     */
    void clearChildren();

    /**
     * Copies (not deep, only pointers) this children into other.
     * @param other
     */
    void copyChildrenTo(const std::shared_ptr<Ast> &other);

    /**
     * Sets the type of the node.
     * @param type
     */
    void setType(AstType type);

    /**
     * Clones the object. Must be overridden by base classes. Returns nullptr if there was an error.
     * @return std::shared_ptr<Ast>
     */
    virtual std::shared_ptr<Ast> clone() const = 0;

    /**
     * Returns the children of this node.
     * @return const std::vector<std::shared_ptr<ast>> &
     */
    const std::vector<std::shared_ptr<Ast>> &getChildren();

    /**
     * Tries to cast node to given type. If cast succeeded returns the casted pointer, other ways nullptr.
     * @tparam Type
     * @param node
     * @return std::shared_ptr<Type>
     */
    template <typename Type> static std::shared_ptr<Type> cast(std::shared_ptr<Ast> node)
    {
        return std::dynamic_pointer_cast<Type>(node);
    }
    
    /**
     * Tries to cast the given node to given type. If cast succeeded returns the casted pointer, other ways nullptr.
     * @tparam Type
     * @param node
     * @return std::shared_ptr<Type>
     */
    template <typename Type> std::shared_ptr<Type> castChildTo(size_t id)
    {
        if (id >= m_children.size())
            return nullptr;
        
        return cast<Type>(getChildren()[id]);
    }

  private:
    AstType m_type;
    std::vector<std::shared_ptr<Ast>> m_children;
};

/**
 * Defines an AST node.
 */
#define LAZY_AST_NODE_DEFINER(AstType, ClassName)                                                                      \
    class ClassName : public Ast                                                                                       \
    {                                                                                                                  \
      public:                                                                                                          \
        ClassName()                                                                                                    \
        {                                                                                                              \
            setType(AstType);                                                                                          \
        }                                                                                                              \
        ~ClassName() override                                                                                          \
        {                                                                                                              \
        }                                                                                                              \
                                                                                                                       \
        std::shared_ptr<Ast> clone() const override                                                                    \
        {                                                                                                              \
            return std::make_shared<ClassName>(*this);                                                                 \
        }                                                                                                              \
    };

LAZY_AST_NODE_DEFINER(AstType::Instruction, InstructionNode)

LAZY_AST_NODE_DEFINER(AstType::Module, ModuleNode)

LAZY_AST_NODE_DEFINER(AstType::VirtualVariable, VirtualVariableNode)

LAZY_AST_NODE_DEFINER(AstType::Type, TypeNode)

LAZY_AST_NODE_DEFINER(AstType::Variable, VariableNode)

#endif // EZPACKER_AST_H
