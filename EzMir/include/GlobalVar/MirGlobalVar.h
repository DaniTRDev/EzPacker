#ifndef EZMIR_MIR_GLOBAL_VAR_H
#define EZMIR_MIR_GLOBAL_VAR_H

#include "EzMirCommon.h"

enum class MirGlobalVarLinkage : uint8_t
{
    External = 0, // Visible globally (standard global)
    Internal,     // Private to this module (static)
    Weak          // Mergeable at link time
};

/**
 * This structure defines global variables. The given MirType MUST be a pointer. This comes from the idea that global
 * variables are POINTERS to the type they were created this.
 *
 * This is handled by the global variable builder.
 */
class MirGlobalVar
{
  public:
    /**
     * Creates the global variable with the given options.
     */
    MirGlobalVar(bool constant,
                 MirId id,
                 MirGlobalVarLinkage linkage,
                 class MirType *type,
                 class MirOperand *initializer,
                 class SourceReference *sourceRef,
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
    class MirOperand *getInitializer() const;

    /**
     * Returns the source reference attached to this global variable.
     */
    class SourceReference *getSourceRef() const;

    /**
     * Returns the name of the global variable.
     */
    const std::pmr::string &getName() const;

  private:
    bool m_constant; // Read-Only
    MirId m_id;
    MirGlobalVarLinkage m_linkage;
    class MirType *m_type;
    class MirOperand *m_initializer; // Only constant values: MirConstantArray, MirFloat, MirInteger.
    class SourceReference *m_sourceRef;
    std::pmr::string m_name;
};

#endif // EZMIR_MIR_GLOBAL_VAR_H