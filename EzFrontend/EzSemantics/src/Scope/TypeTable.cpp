#include "Scope/TypeTable.h"

Type::Type(UnderlyingType underlyingType, UnderlyingTypeSize underlyingTypeSize, const std::string &typeName) :
    m_underlyingType(underlyingType), m_underlyingTypeSize(underlyingTypeSize), m_typeName(typeName)
{
}

UnderlyingType Type::getUnderlyingType() const { return m_underlyingType; }

UnderlyingTypeSize Type::getUnderlyingTypeSize() const { return m_underlyingTypeSize; }

const std::string &Type::getTypeName() const { return m_typeName; }

std::map<std::string, std::shared_ptr<Type>> TypeTable::m_types = {
    { "float", std::make_shared<Type>(UnderlyingType::FloatingPoint, UnderlyingTypeSize::_32bits, "float") },
    { "double", std::make_shared<Type>(UnderlyingType::FloatingPoint, UnderlyingTypeSize::_64bits, "double") },
    { "i8", std::make_shared<Type>(UnderlyingType::Integer, UnderlyingTypeSize::_8bits, "i8") },
    { "i16", std::make_shared<Type>(UnderlyingType::Integer, UnderlyingTypeSize::_16bits, "i16") },
    { "i32", std::make_shared<Type>(UnderlyingType::Integer, UnderlyingTypeSize::_32bits, "i32") },
    { "i64", std::make_shared<Type>(UnderlyingType::Integer, UnderlyingTypeSize::_64bits, "i64") },
    { "i128", std::make_shared<Type>(UnderlyingType::Integer, UnderlyingTypeSize::_128bits, "i128") },
    { "i256", std::make_shared<Type>(UnderlyingType::Integer, UnderlyingTypeSize::_256bits, "i256") },
    { "i512", std::make_shared<Type>(UnderlyingType::Integer, UnderlyingTypeSize::_512bits, "i512") },
    { "pointer", std::make_shared<Type>(UnderlyingType::Pointer, UnderlyingTypeSize::Variable, "ptr") },
    { "string", std::make_shared<Type>(UnderlyingType::String, UnderlyingTypeSize::Variable, "string") },
    { "void", std::make_shared<Type>(UnderlyingType::Void, UnderlyingTypeSize::Invalid, "void") }
};

bool TypeTable::doesTypeExists(const std::string &typeName) { return m_types.contains(typeName); }

std::shared_ptr<Type> TypeTable::getType(const std::string &typeName)
{
    auto it = m_types.find(typeName);
    if (it == m_types.end())
    {
        return nullptr;
    }

    return it->second;
}
