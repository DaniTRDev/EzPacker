#include "SemanticAnnotations/TypeCastAnnotation.h"

TypeCastAnnotation::TypeCastAnnotation(const std::shared_ptr<Symbol> &originalSymbol,
                                       const std::shared_ptr<Type> &castedDataType) :
    m_castedDataType(castedDataType), SymbolAnnotation(originalSymbol)
{
}

bool TypeCastAnnotation::isDoubleToFloat() const
{
    return getSymbol()->getSymbolDataType()->getUnderlyingType() == UnderlyingType::FloatingPoint &&
            m_castedDataType->getUnderlyingType() == UnderlyingType::FloatingPoint &&

            m_castedDataType->getUnderlyingTypeSize() == UnderlyingTypeSize::_32bits &&
            getSymbol()->getSymbolDataType()->getUnderlyingTypeSize() == UnderlyingTypeSize::_64bits;
}

bool TypeCastAnnotation::isDoubleToInteger() const
{
    return getSymbol()->getSymbolDataType()->getUnderlyingType() == UnderlyingType::FloatingPoint &&
            m_castedDataType->getUnderlyingType() == UnderlyingType::Integer &&

            m_castedDataType->getUnderlyingTypeSize() == UnderlyingTypeSize::_64bits;
}

bool TypeCastAnnotation::isExpansion() const
{
    return m_castedDataType->getUnderlyingTypeSize() > getSymbol()->getSymbolDataType()->getUnderlyingTypeSize();
}

bool TypeCastAnnotation::isFloatToDouble() const
{
    return getSymbol()->getSymbolDataType()->getUnderlyingType() == UnderlyingType::FloatingPoint &&
            m_castedDataType->getUnderlyingType() == UnderlyingType::FloatingPoint &&

            getSymbol()->getSymbolDataType()->getUnderlyingTypeSize() == UnderlyingTypeSize::_32bits &&
            m_castedDataType->getUnderlyingTypeSize() == UnderlyingTypeSize::_64bits;
}

bool TypeCastAnnotation::isFloatToInteger() const
{
    return getSymbol()->getSymbolDataType()->getUnderlyingType() == UnderlyingType::FloatingPoint &&
            m_castedDataType->getUnderlyingType() == UnderlyingType::Integer &&

            getSymbol()->getSymbolDataType()->getUnderlyingTypeSize() == UnderlyingTypeSize::_32bits;
}

bool TypeCastAnnotation::isIntegerToDouble() const
{
    return getSymbol()->getSymbolDataType()->getUnderlyingType() == UnderlyingType::Integer &&
            m_castedDataType->getUnderlyingType() == UnderlyingType::FloatingPoint &&

            m_castedDataType->getUnderlyingTypeSize() == UnderlyingTypeSize::_64bits;
}

bool TypeCastAnnotation::isIntegerToFloat() const
{
    return getSymbol()->getSymbolDataType()->getUnderlyingType() == UnderlyingType::Integer &&
            m_castedDataType->getUnderlyingType() == UnderlyingType::FloatingPoint &&

            m_castedDataType->getUnderlyingTypeSize() == UnderlyingTypeSize::_32bits;
}

bool TypeCastAnnotation::isTruncation() const
{
    return m_castedDataType->getUnderlyingTypeSize() < getSymbol()->getSymbolDataType()->getUnderlyingTypeSize();
}

bool TypeCastAnnotation::isIntegerToInteger() const
{
    return getSymbol()->getSymbolDataType()->getUnderlyingType() == UnderlyingType::Integer &&
            m_castedDataType->getUnderlyingType() == UnderlyingType::Integer;
}

const char *TypeCastAnnotation::getAnnotationName() const { return "TypeCastAnnotation"; }

const std::shared_ptr<Type> &TypeCastAnnotation::getCastedDataType() const { return m_castedDataType; }
