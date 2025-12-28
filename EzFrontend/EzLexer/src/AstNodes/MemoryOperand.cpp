#include "AstNodes/MemoryOperand.h"

MemoryOperandAstNode::MemoryOperandAstNode(std::string referencedMemoryDataType) :
    m_referencedMemoryDataType(referencedMemoryDataType)
{
}

AstNodeType MemoryOperandAstNode::getType() const { return AstNodeType::MemoryOperand; }

const char *MemoryOperandAstNode::getAstNodeName() const { return "MemoryOperad"; }

void MemoryOperandAstNode::setReferencedDataType(std::string dataType)
{
    m_referencedMemoryDataType = std::move(dataType);
}

const std::string &MemoryOperandAstNode::getReferencedMemoryDataType() const { return m_referencedMemoryDataType; }

BaseDisplacementMemory::BaseDisplacementMemory(std::shared_ptr<IntegerImmediate> displacement,
                                               std::shared_ptr<Variable> base,
                                               std::string referencedDataType) :
    m_displacement(std::move(displacement)), m_base(std::move(base)),
    MemoryOperandAstNode(std::move(referencedDataType))
{
}

const char *BaseDisplacementMemory::getMemoryOperandTypeName() const { return "BaseDisplacement"; }

MemoryOperandType BaseDisplacementMemory::getMemoryOperandType() const { return MemoryOperandType::BaseDisplacement; }

const std::shared_ptr<IntegerImmediate> &BaseDisplacementMemory::getDisplacement() const { return m_displacement; }

const std::shared_ptr<Variable> &BaseDisplacementMemory::getBase() const { return m_base; }

std::string BaseDisplacementMemory::getAsStr(AstNodeStringMode mode) const
{
    std::string res = std::format("@Memory(type: {} dataType: {}) {{\n",
                                  getMemoryOperandTypeName(),
                                  getReferencedMemoryDataType());

    if (mode == AstNodeStringMode::Default)
    {
        res += "}\n";
        return std::move(res);
    }
    else // Debug
    {
        return std::format("{}(base: {} + displacement: {}) \n}}\n",
                           std::move(res),
                           getBase()->getVariableName(),
                           getDisplacement()->getAsStr(mode));
    }
}

IndexScaleMemory::IndexScaleMemory(std::shared_ptr<IntegerImmediate> scalingFactor,
                                   std::shared_ptr<Variable> index,
                                   std::string referencedDataType) :
    m_scalingFactor(std::move(scalingFactor)), m_index(std::move(index)),
    MemoryOperandAstNode(std::move(referencedDataType))
{
}

const char *IndexScaleMemory::getMemoryOperandTypeName() const { return "IndexScale"; }

MemoryOperandType IndexScaleMemory::getMemoryOperandType() const { return MemoryOperandType::IndexScale; }

const std::shared_ptr<IntegerImmediate> &IndexScaleMemory::getScalingFactor() const { return m_scalingFactor; }

const std::shared_ptr<Variable> &IndexScaleMemory::getIndex() const { return m_index; }

std::string IndexScaleMemory::getAsStr(AstNodeStringMode mode) const
{
    std::string res = std::format("@Memory(type: {} dataType: {}) {{\n",
                                  getMemoryOperandTypeName(),
                                  getReferencedMemoryDataType());

    if (mode == AstNodeStringMode::Default)
    {
        res += "}\n";
        return std::move(res);
    }
    else
    { // Debug
        return std::format("{}({} * {}) \n}}\n",
                           std::move(res),
                           getIndex()->getVariableName(),
                           getScalingFactor()->getAsStr(mode));
    }
}
BaseIndexScaleDisplacementMemory::BaseIndexScaleDisplacementMemory(std::shared_ptr<IntegerImmediate> displacement,
                                                                   std::shared_ptr<IntegerImmediate> scalingFactor,
                                                                   std::shared_ptr<Variable> base,
                                                                   std::shared_ptr<Variable> index,
                                                                   std::string referencedDataType) :
    BaseDisplacementMemory(std::move(displacement), std::move(base), referencedDataType),
    IndexScaleMemory(std::move(scalingFactor), std::move(index), referencedDataType),
    MemoryOperandAstNode(referencedDataType)
{
}

const char *BaseIndexScaleDisplacementMemory::getMemoryOperandTypeName() const { return "BaseIndexScaleDisplacement"; }

MemoryOperandType BaseIndexScaleDisplacementMemory::getMemoryOperandType() const
{
    return MemoryOperandType::BaseIndexScaleDisplacement;
}

std::string BaseIndexScaleDisplacementMemory::getAsStr(AstNodeStringMode mode) const
{
    std::string res = std::format("@Memory(type: {} dataType: {}) {{\n",
                                  getMemoryOperandTypeName(),
                                  getReferencedMemoryDataType());

    if (mode == AstNodeStringMode::Default)
    {
        return std::move(res);
    }
    else
    { // Debug
        return std::format("{}({} + {} * {} + {}) \n}}\n",
                           std::move(res),
                           getBase()->getVariableName(),
                           getIndex()->getVariableName(),
                           getScalingFactor()->getAsStr(mode),
                           getDisplacement()->getAsStr(mode));
    }
}
DirectMemory::DirectMemory(std::shared_ptr<IntegerImmediate> address, std::string referencedDataType) :
    m_address(std::move(address)), MemoryOperandAstNode(std::move(referencedDataType))
{
}

const char *DirectMemory::getMemoryOperandTypeName() const { return "Direct"; }

MemoryOperandType DirectMemory::getMemoryOperandType() const { return MemoryOperandType::Direct; }

const std::shared_ptr<IntegerImmediate> &DirectMemory::getAddress() const { return m_address; }

std::string DirectMemory::getAsStr(AstNodeStringMode mode) const
{
    std::string res = std::format("@Memory(type: {} dataType: {}) {{\n",
                                  getMemoryOperandTypeName(),
                                  getReferencedMemoryDataType());

    if (mode == AstNodeStringMode::Default)
    {
        res += "}\n";
        return std::move(res);
    }
    else // Debug
    {
        return std::format("{}({}) \n}}\n", std::move(res), getAddress()->getAsStr(mode));
    }
}
