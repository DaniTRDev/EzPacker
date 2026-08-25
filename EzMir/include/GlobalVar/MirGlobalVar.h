#ifndef EZMIR_MIR_GLOBAL_VAR_H
#define EZMIR_MIR_GLOBAL_VAR_H

#include "EzMirCommon.h"

/**
 * Linkage visibility and resolution rules for global variables across compilation units.
 */
enum class MirGlobalVarLinkage : uint8_t
{
    External = 0, // Visible globally across translation units (standard external symbol)
    Internal,     // Private to this module / translation unit (static symbol)
    Weak          // Mergeable at link time; can be overridden by a non-weak definition
};

/**
 * Represents a global variable in MIR.
 * The variable's MirType is always a pointer type pointing to the underlying data type,
 * reflecting memory-location semantics in MIR.
 */
class MirGlobalVar
{
  public:
    /**
     * Constructs a global variable descriptor with constness, ID, linkage, pointer type, initializer operand,
     * source reference, and symbol name.
     */
    MirGlobalVar(bool constant,
                 MirId id,
                 MirGlobalVarLinkage linkage,
                 class MirType *type,
                 class MirOperand *initializer,
                 class SourceReference *sourceRef,
                 const std::pmr::string &name);

    /**
     * Checks if this global variable is read-only (constant).
     */
    bool isConstant() const;

    /**
     * Retrieves the unique identifier of this global variable.
     */
    MirId getId() const;

    /**
     * Retrieves the symbol linkage and visibility specification.
     */
    MirGlobalVarLinkage getLinkage() const;

    /**
     * Retrieves the pointer type associated with this global variable.
     */
    MirType *getType() const;

    /**
     * Retrieves the constant initializer operand (e.g., MirInteger, MirFloat, or MirConstantArray), if any.
     */
    class MirOperand *getInitializer() const;

    /**
     * Retrieves the source code reference associated with this global definition.
     */
    class SourceReference *getSourceRef() const;

    /**
     * Retrieves the symbolic name of the global variable.
     */
    const std::pmr::string &getName() const;

  private:
    /**
     * Indicates whether the global variable is immutable/read-only.
     */
    bool m_constant;

    /**
     * Unique MIR identifier for this global variable.
     */
    MirId m_id;

    /**
     * Linkage visibility of the symbol.
     */
    MirGlobalVarLinkage m_linkage;

    /**
     * Pointer type referring to the value type of the global variable.
     */
    class MirType *m_type;

    /**
     * Initializer expression operand (restricted to constant values).
     */
    class MirOperand *m_initializer;

    /**
     * Source location reference for diagnostics.
     */
    class SourceReference *m_sourceRef;

    /**
     * Identifier name of the global variable in the symbol table.
     */
    std::pmr::string m_name;
};

#endif // EZMIR_MIR_GLOBAL_VAR_H