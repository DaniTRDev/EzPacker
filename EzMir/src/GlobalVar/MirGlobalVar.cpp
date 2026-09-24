#include "GlobalVar/MirGlobalVar.h"
#include "Operand/MirOperand.h"
#include "SourceManager/SourceManager.h"
#include "Type/MirType.h"

/**
 * Initializes a new global variable with its attributes, linkage, pointer type, and optional initializer.
 */
MirGlobalVar::MirGlobalVar(bool constant,
                           MirId id,
                           MirGlobalVarLinkage linkage,
                           MirType *type,
                           MirOperand *initializer,
                           SourceReference *sourceRef,
                           std::pmr::string name) :
    m_constant(constant), m_id(id), m_linkage(linkage), m_type(type), m_initializer(initializer),
    m_sourceRef(sourceRef), m_name(std::move(name))
{
}

/**
 * Returns whether the global variable is constant (read-only).
 */
bool MirGlobalVar::isConstant() const { return m_constant; }

/**
 * Returns the unique MIR identifier of the global variable.
 */
MirId MirGlobalVar::getId() const { return m_id; }

/**
 * Returns the linkage specification for cross-module symbol resolution.
 */
MirGlobalVarLinkage MirGlobalVar::getLinkage() const { return m_linkage; }

/**
 * Returns the pointer type associated with the global variable's memory address.
 */
MirType *MirGlobalVar::getType() const { return m_type; }

/**
 * Returns the constant initializer operand attached to the global variable.
 */
MirOperand *MirGlobalVar::getInitializer() const { return m_initializer; }

/**
 * Returns the source location reference for diagnostics.
 */
SourceReference *MirGlobalVar::getSourceRef() const { return m_sourceRef; }

/**
 * Returns the symbolic name of the global variable.
 */
const std::pmr::string &MirGlobalVar::getName() const { return m_name; }