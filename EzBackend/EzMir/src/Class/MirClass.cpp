#include "Class/MirClass.h"

MirClass::MirClass(MirClass *parentClass,
                   MirId id,
                   MirType *type,
                   const std::pmr::string &name,
                   std::pmr::vector<MirClassField> fields,
                   std::pmr::vector<MirFunction *> vTable,
                   SourceReference *sourceRef) :
    m_parentClass(parentClass), m_id(id), m_type(type), m_name(name), m_sourceRef(sourceRef),
    m_fields(std::move(fields)), m_vTable(std::move(vTable))
{
}

MirClass *MirClass::getParentClass() const { return m_parentClass; }

MirId MirClass::getId() const { return m_id; }

MirType *MirClass::getType() const { return m_type; }

SourceReference *MirClass::getSourceRef() const { return m_sourceRef; }

void MirClass::appendField(MirType *type, const std::pmr::string &name)
{
    m_fields.push_back(MirClassField{ .m_type = type, .m_offset = -1, .m_name = name });
}

void MirClass::appendMethod(MirFunction *func) { m_vTable.push_back(func); }

const std::pmr::string &MirClass::getName() const { return m_name; }

const std::pmr::vector<MirClassField> &MirClass::getFields() const { return m_fields; }

std::pmr::vector<MirClassField> *MirClass::getFieldsPtr() { return &m_fields; }

const std::pmr::vector<MirFunction *> &MirClass::getVTable() const { return m_vTable; }
