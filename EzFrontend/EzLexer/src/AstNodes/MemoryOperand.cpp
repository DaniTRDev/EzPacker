#include "AstNodes/MemoryOperand.h"

MemoryOperandAstNode::MemoryOperandAstNode(std::string_view referencedMemoryDataType) :
    m_referencedMemoryDataType(referencedMemoryDataType)
{
}

AstNodeType MemoryOperandAstNode::getType() const { return AstNodeType::MemoryOperand; }

bool MemoryOperandAstNode::accept(AstNodeVisitor *visitor)
{
    if (visitor)
    {
        return visitor->visit(this);
    }
    return false;
}

const char *MemoryOperandAstNode::getAstNodeName() const { return "MemoryOperand"; }

void MemoryOperandAstNode::setReferencedDataType(std::string_view dataType) { m_referencedMemoryDataType = dataType; }

const std::string_view &MemoryOperandAstNode::getReferencedMemoryDataTypeStr() const
{
    return m_referencedMemoryDataType;
}

BaseDisplacementMemory::BaseDisplacementMemory(IntegerImmediate *displacement,
                                               Variable *base,
                                               std::string_view referencedDataType) :
    m_displacement(displacement), m_base(base), MemoryOperandAstNode(referencedDataType)
{
}

const char *BaseDisplacementMemory::getMemoryOperandTypeName() const { return "BaseDisplacement"; }

IntegerImmediate *BaseDisplacementMemory::getDisplacement() const { return m_displacement; }

MemoryOperandType BaseDisplacementMemory::getMemoryOperandType() const { return MemoryOperandType::BaseDisplacement; }

Variable *BaseDisplacementMemory::getBase() const { return m_base; }

std::string BaseDisplacementMemory::getAsStr(AstNodeStringMode mode) const
{
    std::string res = std::format("@Memory(type: {} dataType: {}) {{\n",
                                  getMemoryOperandTypeName(),
                                  getReferencedMemoryDataTypeStr());

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

IndexScaleMemory::IndexScaleMemory(IntegerImmediate *scalingFactor,
                                   Variable *index,
                                   std::string_view referencedDataType) :
    m_scalingFactor(scalingFactor), m_index(index), MemoryOperandAstNode(referencedDataType)
{
}

const char *IndexScaleMemory::getMemoryOperandTypeName() const { return "IndexScale"; }

IntegerImmediate *IndexScaleMemory::getScalingFactor() const { return m_scalingFactor; }

MemoryOperandType IndexScaleMemory::getMemoryOperandType() const { return MemoryOperandType::IndexScale; }

Variable *IndexScaleMemory::getIndex() const { return m_index; }

std::string IndexScaleMemory::getAsStr(AstNodeStringMode mode) const
{
    std::string res = std::format("@Memory(type: {} dataType: {}) {{\n",
                                  getMemoryOperandTypeName(),
                                  getReferencedMemoryDataTypeStr());

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
BaseIndexScaleDisplacementMemory::BaseIndexScaleDisplacementMemory(IntegerImmediate *displacement,
                                                                   IntegerImmediate *scalingFactor,
                                                                   Variable *base,
                                                                   Variable *index,
                                                                   std::string_view referencedDataType) :
    BaseDisplacementMemory(displacement, base, referencedDataType),
    IndexScaleMemory(scalingFactor, index, referencedDataType), MemoryOperandAstNode(referencedDataType)
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
                                  getReferencedMemoryDataTypeStr());

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

DirectMemory::DirectMemory(IntegerImmediate *address, std::string_view referencedDataType) :
    m_address(std::move(address)), MemoryOperandAstNode(std::move(referencedDataType))
{
}

const char *DirectMemory::getMemoryOperandTypeName() const { return "Direct"; }

MemoryOperandType DirectMemory::getMemoryOperandType() const { return MemoryOperandType::Direct; }

IntegerImmediate *DirectMemory::getAddress() const { return m_address; }

std::string DirectMemory::getAsStr(AstNodeStringMode mode) const
{
    std::string res = std::format("@Memory(type: {} dataType: {}) {{\n",
                                  getMemoryOperandTypeName(),
                                  getReferencedMemoryDataTypeStr());

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
