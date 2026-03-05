/**
 * @file MemoryOperand.h
 * @brief AST nodes for the various memory-addressing modes.
 *
 * MemoryOperandAstNode is the abstract base class; concrete subclasses are:
 *   - BaseDisplacementMemory              — `type (%base + disp)`
 *   - IndexScaleMemory                    — `type (, %idx, scale)`
 *   - BaseIndexScaleDisplacementMemory    — combines base+disp with idx*scale
 *   - DirectMemory                        — `type (address)`
 *
 * Each variant records the referenced data type (e.g. i64, i8) and the
 * addressing components.  The MemoryLowerer translates these into MirMemory
 * operands during the AST-to-MIR lowering phase.
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
     * Creates the node with the given data type.
     * @param referencedMemoryDataType
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
     * Returns the name of the operand type.
     * @return const char*
     */
    virtual const char *getMemoryOperandTypeName() const = 0;

    /**
     * Returns the type of memory operand.
     * @return MemoryOperandType
     */
    virtual MemoryOperandType getMemoryOperandType() const = 0;

    /**
     * Sets the referenced data type.
     * @param dataType
     */
    void setReferencedDataType(std::string_view dataType);

    /**
     * Returns the underlying type of the referenced memory region: uint64_t* -> memory referenced is an uint64_t.
     * @return const std::string &
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
     * Creates the memory operand with the given displacement, base and referenced data type.
     * @param displacement
     * @param base
     * @param referencedDataType
     */
    BaseDisplacementMemory(IntegerImmediate *displacement, Variable *base, std::string_view referencedDataType);

    /**
     * Returns "BaseDisplacement".
     * @return const char*
     */
    const char *getMemoryOperandTypeName() const override;

    /**
     * Returns the displacement of the memory address.
     * @return IntegerImmediate*
     */
    IntegerImmediate *getDisplacement() const;

    /**
     * Returns MemoryOperandType::BaseDisplacement.
     * @return MemoryOperandType
     */
    MemoryOperandType getMemoryOperandType() const override;

    /**
     * Returns the base of the memory address.
     * @return Variable*
     */
    Variable *getBase() const;

    /**
     * Returns this object in a formatted string (human readable). The quantity of the information included in the
     * formatted string depends on mode. See AstNodeStringMode for more information. If mode is set to default,
     * referenced data type and addressing format will be the only information shown. If mode is set to debug, memory
     * address operands will also be shown.
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;

  private:
    IntegerImmediate *m_displacement;
    Variable *m_base;
};

class IndexScaleMemory : virtual public MemoryOperandAstNode
{
  public:
    /**
     * Creates the memory operand with the given scaling factor, index and referenced data type.
     * @param scalingFactor
     * @param index
     * @param referencedDataType
     */
    IndexScaleMemory(IntegerImmediate *scalingFactor, Variable *index, std::string_view referencedDataType);

    /**
     * Returns "IndexScale".
     * @return const char*
     */
    const char *getMemoryOperandTypeName() const override;

    /**
     * Returns the scaling factor, if any, for this memory reference.
     * @return IntegerImmediate *
     */
    IntegerImmediate *getScalingFactor() const;

    /**
     * Returns MemoryOperandType::IndexScale.
     * @return MemoryOperandType
     */
    MemoryOperandType getMemoryOperandType() const override;

    /**
     * Returns the index, if any, for this memory reference.
     * @return Variable *
     */
    Variable *getIndex() const;

    /**
     * Returns this object in a formatted string (human readable). The quantity of the information included in the
     * formatted string depends on mode. See AstNodeStringMode for more information. If mode is set to default,
     * referenced data type and addressing format will be the only information shown. If mode is set to debug, memory
     * address operands will also be shown.
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;

  private:
    IntegerImmediate *m_scalingFactor;
    Variable *m_index;
};

class BaseIndexScaleDisplacementMemory : public IndexScaleMemory, public BaseDisplacementMemory
{
  public:
    /**
     * Creates the memory operand with the given displacement, scaling factor, base, index and referenced data type.
     * @param displacement
     * @param scalingFactor
     * @param base
     * @param index
     * @param referencedDataType
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

    /**
     * Returns this object in a formatted string (human readable). The quantity of the information included in the
     * formatted string depends on mode. See AstNodeStringMode for more information. If mode is set to default,
     * referenced data type and addressing format will be the only information shown. If mode is set to debug, memory
     * address operands will also be shown.
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;
};

class DirectMemory : public MemoryOperandAstNode
{
  public:
    /**
     * Creates the memory operand with the given address and referenced data type.
     * @param address
     * @param referencedDataType
     */
    DirectMemory(IntegerImmediate *address, std::string_view referencedDataType);

    /**
     * Returns "BaseDisplacement".
     * @return const char*
     */
    const char *getMemoryOperandTypeName() const override;

    /**
     * Returns MemoryOperandType::BaseDisplacement.
     * @return MemoryOperandType
     */
    MemoryOperandType getMemoryOperandType() const override;

    /**
     * Returns the address being referenced.
     * @return IntegerImmediate *
     */
    IntegerImmediate *getAddress() const;

    /**
     * Returns this object in a formatted string (human readable). The quantity of the information included in the
     * formatted string depends on mode. See AstNodeStringMode for more information. If mode is set to default,
     * referenced data type and addressing format will be the only information shown. If mode is set to debug, memory
     * address operands will also be shown.
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;

  private:
    IntegerImmediate *m_address;
};

#endif // EZPACKER_MEMORYOPERAND_H
