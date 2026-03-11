#include "SemanticAnnotations/TypeCastAnnotation.h"

TypeCastAnnotation::TypeCastAnnotation(Symbol *originalSymbol, Type *castedDataType) :
    SymbolAnnotation(originalSymbol), DataTypeAnnotation(castedDataType)
{
}

bool TypeCastAnnotation::isDoubleToFloat() const
{
    return getSymbol()->getSymbolDataType()->getUnderlyingType() == UnderlyingType::FloatingPoint &&
            getDataType()->getUnderlyingType() == UnderlyingType::FloatingPoint &&

            getDataType()->getUnderlyingTypeSize() == UnderlyingTypeSize::_32bits &&
            getSymbol()->getSymbolDataType()->getUnderlyingTypeSize() == UnderlyingTypeSize::_64bits;
}

bool TypeCastAnnotation::isDoubleToInteger() const
{
    return getSymbol()->getSymbolDataType()->getUnderlyingType() == UnderlyingType::FloatingPoint &&
            getDataType()->getUnderlyingType() == UnderlyingType::Integer &&

            getDataType()->getUnderlyingTypeSize() == UnderlyingTypeSize::_64bits;
}

bool TypeCastAnnotation::isExpansion() const
{
    return getDataType()->getUnderlyingTypeSize() > getSymbol()->getSymbolDataType()->getUnderlyingTypeSize();
}

bool TypeCastAnnotation::isFloatToDouble() const
{
    return getSymbol()->getSymbolDataType()->getUnderlyingType() == UnderlyingType::FloatingPoint &&
            getDataType()->getUnderlyingType() == UnderlyingType::FloatingPoint &&

            getSymbol()->getSymbolDataType()->getUnderlyingTypeSize() == UnderlyingTypeSize::_32bits &&
            getDataType()->getUnderlyingTypeSize() == UnderlyingTypeSize::_64bits;
}

bool TypeCastAnnotation::isFloatToInteger() const
{
    return getSymbol()->getSymbolDataType()->getUnderlyingType() == UnderlyingType::FloatingPoint &&
            getDataType()->getUnderlyingType() == UnderlyingType::Integer &&

            getSymbol()->getSymbolDataType()->getUnderlyingTypeSize() == UnderlyingTypeSize::_32bits;
}

bool TypeCastAnnotation::isIntegerToDouble() const
{
    return getSymbol()->getSymbolDataType()->getUnderlyingType() == UnderlyingType::Integer &&
            getDataType()->getUnderlyingType() == UnderlyingType::FloatingPoint &&

            getDataType()->getUnderlyingTypeSize() == UnderlyingTypeSize::_64bits;
}

bool TypeCastAnnotation::isIntegerToFloat() const
{
    return getSymbol()->getSymbolDataType()->getUnderlyingType() == UnderlyingType::Integer &&
            getDataType()->getUnderlyingType() == UnderlyingType::FloatingPoint &&

            getDataType()->getUnderlyingTypeSize() == UnderlyingTypeSize::_32bits;
}

bool TypeCastAnnotation::isTruncation() const
{
    return getDataType()->getUnderlyingTypeSize() < getSymbol()->getSymbolDataType()->getUnderlyingTypeSize();
}

bool TypeCastAnnotation::isIntegerToInteger() const
{
    return getSymbol()->getSymbolDataType()->getUnderlyingType() == UnderlyingType::Integer &&
            getDataType()->getUnderlyingType() == UnderlyingType::Integer;
}

const char *TypeCastAnnotation::getAnnotationName() const { return "TypeCastAnnotation"; }
