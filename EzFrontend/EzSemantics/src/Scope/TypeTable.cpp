#include "Scope/TypeTable.h"

Type::Type(UnderlyingType underlyingType, UnderlyingTypeSize underlyingTypeSize, const std::string_view &typeName) :
    m_sourceRef(), m_underlyingType(underlyingType), m_underlyingTypeSize(underlyingTypeSize), m_typeName(typeName)
{
}

const SourceReference &Type::getSourceRef() const { return m_sourceRef; }

UnderlyingType Type::getUnderlyingType() const { return m_underlyingType; }

UnderlyingTypeSize Type::getUnderlyingTypeSize() const { return m_underlyingTypeSize; }

void Type::setSourceRef(const SourceReference &ref) { m_sourceRef = ref; }

void Type::setSubTypes(const std::vector<Type *> &types) { m_subTypes = types; }

const std::string_view &Type::getTypeName() const { return m_typeName; }

const std::vector<Type *> &Type::getSubTypes() const { return m_subTypes; }

TypeTable::TypeTable()
{
    addType(UnderlyingType::FloatingPoint, UnderlyingTypeSize::_32bits, "float");
    addType(UnderlyingType::FloatingPoint, UnderlyingTypeSize::_64bits, "double");
    addType(UnderlyingType::Integer, UnderlyingTypeSize::_8bits, "i8");
    addType(UnderlyingType::Integer, UnderlyingTypeSize::_16bits, "i16");
    addType(UnderlyingType::Integer, UnderlyingTypeSize::_32bits, "i32");
    addType(UnderlyingType::Integer, UnderlyingTypeSize::_64bits, "i64");
    addType(UnderlyingType::Integer, UnderlyingTypeSize::_128bits, "i128");
    addType(UnderlyingType::Integer, UnderlyingTypeSize::_256bits, "i256");
    addType(UnderlyingType::Integer, UnderlyingTypeSize::_512bits, "i512");
    addType(UnderlyingType::String, UnderlyingTypeSize::Variable, "string");
    addType(UnderlyingType::Void, UnderlyingTypeSize::Invalid, "void");
}

bool TypeTable::doesTypeExists(const std::string_view &typeName) { return m_types.contains(typeName); }

std::shared_ptr<Type> TypeTable::addType(UnderlyingType underlyingType,
                                         UnderlyingTypeSize underlyingTypeSize,
                                         const std::string_view &typeName)
{
    if (doesTypeExists(typeName))
    {
        return nullptr;
    }

    std::shared_ptr<Type> type = std::make_shared<Type>(underlyingType, underlyingTypeSize, typeName);

    m_types.emplace(typeName, type);
    return type;
}

std::shared_ptr<Type> TypeTable::addModuleType(const std::string_view &moduleName, const std::vector<Type *> &subTypes)
{
    if (doesTypeExists(moduleName))
    {
        return nullptr;
    }

    std::shared_ptr<Type> type =
            std::make_shared<Type>(UnderlyingType::Module, UnderlyingTypeSize::Variable, moduleName);
    type->setSubTypes(subTypes);

    m_types.emplace(moduleName, type);
    return type;
}

std::shared_ptr<Type> TypeTable::getType(const std::string_view &typeName)
{
    auto it = m_types.find(typeName);
    if (it == m_types.end())
    {
        return nullptr;
    }

    return it->second;
}

std::shared_ptr<Type> TypeTable::getDefaultType() { return getType("i64"); }

const std::map<std::string_view, std::shared_ptr<Type>> &TypeTable::getTypeMap() const { return m_types; }
