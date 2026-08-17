#include "Class/MirClass.h"
#include "Function/MirFunction.h"
#include "Operand/MirOperands.h"
#include "SourceManager/SourceManager.h"
#include "Type/MirType.h"

MirClass::MirClass(MirClass *parentClass,
                   MirId id,
                   MirType *type,
                   const std::pmr::string &name,
                   std::pmr::map<std::pmr::string, MirClassField *> fieldNameToField,
                   std::pmr::vector<MirClassField *> fields,
                   std::pmr::vector<MirClassMethod *> vTable,
                   SourceReference *sourceRef) :
    m_parentClass(parentClass), m_id(id), m_type(type), m_name(name), m_fieldNameToField(std::move(fieldNameToField)),
    m_sourceRef(sourceRef), m_fields(std::move(fields)), m_vTable(std::move(vTable))
{
}

MirClass *MirClass::getParentClass() const { return m_parentClass; }

MirClassField *MirClass::getFieldById(size_t index) const
{
    if (index >= m_fields.size())
    {
        return nullptr;
    }

    return m_fields[index];
}

MirClassField *MirClass::getFieldByName(const std::string_view &name) const
{
    auto it = m_fieldNameToField.find(std::pmr::string(name));

    if (it != m_fieldNameToField.end())
        return it->second;

    return nullptr;
}

MirClassMethod *MirClass::getMethodById(size_t index) const
{
    if (index >= m_vTable.size())
    {
        return nullptr;
    }

    return m_vTable[index];
}

MirClassMethod *MirClass::getMethodBySignature(MirType *returnType,
                                               std::vector<MirType *> argsTypes,
                                               const std::string_view &name) const
{
    for (auto &method : m_vTable)
    {
        MirFunction *func = method->m_func;

        // Quickly discard by return type.
        if (func->getReturnType()->getId() != returnType->getId())
            continue;

        // Quickly discard by argument types.
        auto funcParams = func->getParameters();
        if (argsTypes.size() != funcParams.size())
            continue;

        bool matchedParams = true;
        size_t i = 0;

        for (auto &param : funcParams)
        {
            if (param->getMirType()->getId() != argsTypes[i]->getId())
            {
                matchedParams = false;
                break;
            }

            i++;
        }

        // Quickly discard by name.
        if (func->getName() != name)
            continue;

        return method;
    }
    return nullptr;
}

MirId MirClass::getId() const { return m_id; }

MirType *MirClass::getType() const { return m_type; }

size_t MirClass::getFieldCount() const { return m_fields.size(); }

size_t MirClass::getVTableSize() const { return m_vTable.size(); }

SourceReference *MirClass::getSourceRef() const { return m_sourceRef; }

const std::pmr::string &MirClass::getName() const { return m_name; }

const std::pmr::vector<MirClassField *> &MirClass::getFields() const { return m_fields; }

const std::pmr::vector<MirClassMethod *> &MirClass::getVTable() const { return m_vTable; }
