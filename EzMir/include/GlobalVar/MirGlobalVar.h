#ifndef EZPACKER_MIRMirGlobalVar_H
#define EZPACKER_MIRMirGlobalVar_H

#include "EzMirCommon.h"
#include "Operand/MirOperand.h"
#include "Type/MirType.h"

enum class MirGlobalVarLinkage
{
    External = 0, // Visible globally (standard global)
    Internal,     // Private to this module (static)
    Weak          // Mergeable at link time
};

class MirGlobalVar
{
  public:
    /**
     * Creates the global variable with the given options.
     * @param constant
     * @param id
     * @param linkage
     * @param type
     * @param initializer
     * @param sourceRef
     * @param name
     */
    MirGlobalVar(bool constant,
                 MirId id,
                 MirGlobalVarLinkage linkage,
                 MirType *type,
                 MirOperand *initializer,
                 SourceReference *sourceRef,
                 const std::pmr::string &name);

    /**
     * Returns true if this global variable is constant (read-only).
     */
    bool isConstant() const;

    /**
     * Returns the ID of this global variable.
     */
    MirId getId() const;

    /**
     * Returns the linkage type for this variable.
     */
    MirGlobalVarLinkage getLinkage() const;

    /**
     * Returns the type of the global variable.
     */
    MirType *getType() const;

    /**
     * Returns the initializer for this global variable.
     */
    MirOperand *getInitializer() const;

    /**
     * Returns the source reference attached to this global variable.
     */
    SourceReference *getSourceRef() const;

    /**
     * Returns the name of the global variable.
     */
    const std::pmr::string &getName() const;

  private:
    bool m_constant; // Read-Only
    MirId m_id;
    MirGlobalVarLinkage m_linkage;
    MirType *m_type;
    MirOperand *m_initializer; // Only constant values: MirConstantArray, MirFloat, MirInteger.
    SourceReference *m_sourceRef;
    std::pmr::string m_name;
};

#endif // EZPACKER_MirGlobalVar_H