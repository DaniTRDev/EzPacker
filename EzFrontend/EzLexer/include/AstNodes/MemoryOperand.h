/**
 * @file MemoryOperand.h
 * @brief AST nodes for frontend-visible memory addressing modes.
 *
 * All memory operands share two common pieces of information:
 * - the referenced element type (for example `i8`, `i32`, `i64`), and
 * - one of the supported addressing layouts.
 *
 * Public parser grammar accepted today:
 * - BaseDisplacement:             `type (%base +/- displacement)`
 * - BaseIndexScaleDisplacement:   `type (%base, %index, scale, displacement)`
 * - IndexScale:                   `type (, %index, scale)`
 * - Direct:                       `type (address)`
 *
 * The frontend stores the components structurally; it does not attempt to map
 * them to target-specific addressing modes yet.
 */
#ifndef EZPACKER_MEMORYOPERAND_H
#define EZPACKER_MEMORYOPERAND_H

#include "EzLexerCommon.h"
#include "AstNode/AstNodeVisitor.h"
#include "AstNodes/Variable.h"
#include "AstNodes/ImmediateOperand.h"

enum class MemoryOperandType
{
    Invalid = 0,
    BaseDisplacement,
    BaseIndexScaleDisplacement,
    Direct, /* Its use is allowed, but it will surely be transformed by the backend into another reference (mostly
             * IPRelative)
             */
    IndexScale
    // IPRelative is also a memory operand type, but it will not be available in the frontend.
};

class MemoryOperandAstNode : public AstNode
{
  public:
    /**
     * Creates a memory operand with the given referenced element type.
     *
     * The type spelling is stored as parsed and later resolved by EzSemantics.
     */
    MemoryOperandAstNode(std::string_view referencedMemoryDataType);

    /**
     * Returns AstNodeType::MemoryOperand.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

    /**
     * Accepts the given visitor and calls its internal visit method with the correct node type. Returns
     * the result of visit.
     * @param visitor
     * @return bool
     */
    bool accept(AstNodeVisitor *visitor) override;
    
    /**
     * Returns "MemoryOperand"
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Returns a stable string describing the concrete memory operand subclass.
     */
    virtual const char *getMemoryOperandTypeName() const = 0;

    /**
     * Returns the concrete addressing kind.
     */
    virtual MemoryOperandType getMemoryOperandType() const = 0;

    /**
     * Updates the referenced element type spelling.
     *
     * Parsers typically construct the node first and assign the type after the
     * specific addressing mode has been recognized.
     */
    void setReferencedDataType(std::string_view dataType);

    /**
     * Returns the parsed element type spelling for the referenced memory.
     *
     * Example: in `i64 (%base+8)`, this returns `i64`.
     */
    const std::string_view &getReferencedMemoryDataTypeStr() const;

  private:
    /**
     * Real type of the memory being referenced: uint64_t* -> memory referenced is an uint64_t.
     */
    std::string_view m_referencedMemoryDataType;
};

class BaseDisplacementMemory : virtual public MemoryOperandAstNode
{
  public:
    /**
     * Memory operand of the form `type (%base +/- displacement)`.
     */
    BaseDisplacementMemory(IntegerImmediate *displacement, Variable *base, std::string_view referencedDataType);

    /**
     * Returns "BaseDisplacement".
     * @return const char*
     */
    const char *getMemoryOperandTypeName() const override;

    /**
     * Returns the parsed integer displacement.
     *
     * The sign, when present, is represented inside the immediate node value.
     */
    IntegerImmediate *getDisplacement() const;

    /**
     * Returns MemoryOperandType::BaseDisplacement.
     * @return MemoryOperandType
     */
    MemoryOperandType getMemoryOperandType() const override;

    /**
     * Returns the base variable used to form the address.
     */
    Variable *getBase() const;
    
  private:
    IntegerImmediate *m_displacement;
    Variable *m_base;
};

class IndexScaleMemory : virtual public MemoryOperandAstNode
{
  public:
    /**
     * Memory operand of the form `type (, %index, scale)`.
     */
    IndexScaleMemory(IntegerImmediate *scalingFactor, Variable *index, std::string_view referencedDataType);

    /**
     * Returns "IndexScale".
     * @return const char*
     */
    const char *getMemoryOperandTypeName() const override;

    /**
     * Returns the parsed scale factor.
     */
    IntegerImmediate *getScalingFactor() const;

    /**
     * Returns MemoryOperandType::IndexScale.
     * @return MemoryOperandType
     */
    MemoryOperandType getMemoryOperandType() const override;

    /**
     * Returns the index variable used in the address calculation.
     */
    Variable *getIndex() const;
    
  private:
    IntegerImmediate *m_scalingFactor;
    Variable *m_index;
};

class BaseIndexScaleDisplacementMemory : public IndexScaleMemory, public BaseDisplacementMemory
{
  public:
    /**
     * Memory operand of the form
     * `type (%base, %index, scale, displacement)`.
     */
    BaseIndexScaleDisplacementMemory(IntegerImmediate *displacement,
                                     IntegerImmediate *scalingFactor,
                                     Variable *base,
                                     Variable *index,
                                     std::string_view referencedDataType);

    /**
     * Returns "BaseIndexScaleDisplacement".
     * @return const char*
     */
    const char *getMemoryOperandTypeName() const override;

    /**
     * Returns MemoryOperandType::BaseIndexScaleDisplacement.
     * @return MemoryOperandType
     */
    MemoryOperandType getMemoryOperandType() const override;
    
};

class DirectMemory : public MemoryOperandAstNode
{
  public:
    /**
     * Memory operand of the form `type (address)`.
     */
    DirectMemory(IntegerImmediate *address, std::string_view referencedDataType);

    /**
     * Returns "Direct".
     * @return const char*
     */
    const char *getMemoryOperandTypeName() const override;

    /**
     * Returns MemoryOperandType::Direct.
     * @return MemoryOperandType
     */
    MemoryOperandType getMemoryOperandType() const override;

    /**
     * Returns the immediate address value.
     */
    IntegerImmediate *getAddress() const;
    
  private:
    IntegerImmediate *m_address;
};

#endif // EZPACKER_MEMORYOPERAND_H
