#include "GlobalVar/MirGlobalVar.h"

MirGlobalVar::MirGlobalVar(bool constant,
                           MirId id,
                           MirGlobalVarLinkage linkage,
                           MirType *type,
                           MirOperand *initializer,
                           SourceReference *sourceRef,
                           const std::pmr::string &name) :
    m_constant(constant), m_id(id), m_linkage(linkage), m_type(type), m_initializer(initializer),
    m_sourceRef(sourceRef), m_name(name)
{
}

bool MirGlobalVar::isConstant() const { return m_constant; }

MirId MirGlobalVar::getId() const { return m_id; }

MirGlobalVarLinkage MirGlobalVar::getLinkage() const { return m_linkage; }

MirType *MirGlobalVar::getType() const { return m_type; }

MirOperand *MirGlobalVar::getInitializer() const
{
    return m_initializer;
}

SourceReference *MirGlobalVar::getSourceRef() const { return m_sourceRef; }

const std::pmr::string &MirGlobalVar::getName() const { return m_name; }