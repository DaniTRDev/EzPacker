#ifndef EZPACKER_MEMORYOPERAND_H
#define EZPACKER_MEMORYOPERAND_H

#include "EzLexerCommon.h"
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
    MemoryOperandAstNode(std::string referencedMemoryDataType);

    /**
     * Returns AstNodeType::MemoryOperand.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

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
     * Sets the refenced data type.
     * @param dataType
     */
    void setReferencedDataType(std::string dataType);

    /**
     * Returns the underlying type of the referenced memory region: uint64_t* -> memory referenced is an uint64_t.
     * @return const std::string &
     */
    const std::string &getReferencedMemoryDataTypeStr() const;
    
  private:
    /**
     * Real type of the memory being referenced: uint64_t* -> memory referenced is an uint64_t.
     */
    std::string m_referencedMemoryDataType;
};

class BaseDisplacementMemory : virtual public MemoryOperandAstNode
{
  public:
    /**
     * Creates the memory operand with the given displacement, base and referenced data type.
     */
    BaseDisplacementMemory(std::shared_ptr<IntegerImmediate> displacement,
                           std::shared_ptr<Variable> base,
                           std::string referencedDataType);

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
     * Returns the displacement of the memory address.
     * @return const std::shared_ptr<IntegerImmediate> &
     */
    const std::shared_ptr<IntegerImmediate> &getDisplacement() const;

    /**
     * Returns the base of the memory address.
     * @return const std::shared_ptr<Variable>
     */
    const std::shared_ptr<Variable> &getBase() const;
    
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
    std::shared_ptr<IntegerImmediate> m_displacement;
    std::shared_ptr<Variable> m_base;
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
    IndexScaleMemory(std::shared_ptr<IntegerImmediate> scalingFactor,
                     std::shared_ptr<Variable> index,
                     std::string referencedDataType);

    /**
     * Returns "IndexScale".
     * @return const char*
     */
    const char *getMemoryOperandTypeName() const override;

    /**
     * Returns MemoryOperandType::BaseDisplacement.
     * @return MemoryOperandType
     */
    MemoryOperandType getMemoryOperandType() const override;

    /**
     * Returns the scaling factor, if any, for this memory reference.
     * @return const std::shared_ptr<IntegerImmediate> &
     */
    const std::shared_ptr<IntegerImmediate> &getScalingFactor() const;

    /**
     * Returns the index, if any, for this memory reference.
     * @return const std::shared_ptr<Variable> &
     */
    const std::shared_ptr<Variable> &getIndex() const;

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
    std::shared_ptr<IntegerImmediate> m_scalingFactor;
    std::shared_ptr<Variable> m_index;
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
    BaseIndexScaleDisplacementMemory(std::shared_ptr<IntegerImmediate> displacement,
                                     std::shared_ptr<IntegerImmediate> scalingFactor,
                                     std::shared_ptr<Variable> base,
                                     std::shared_ptr<Variable> index,
                                     std::string referencedDataType);

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
    DirectMemory(std::shared_ptr<IntegerImmediate> address, std::string referencedDataType);

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
     * @return const std::shared_ptr<IntegerImmediate> &
     */
    const std::shared_ptr<IntegerImmediate> &getAddress() const;
    
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
    std::shared_ptr<IntegerImmediate> m_address;
};

#endif // EZPACKER_MEMORYOPERAND_H
