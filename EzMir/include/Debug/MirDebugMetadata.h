#ifndef EZMIR_MIR_DEBUG_METADATA_H
#define EZMIR_MIR_DEBUG_METADATA_H

#include "EzMirCommon.h"
#include "Debug/DebugInfo.h"
#include <optional>
#include <string>
#include <vector>

/**
 * Links a source-level function parameter or local variable to its MIR storage
 * (either a stack frame object ID or a virtual register ID).
 */
struct MirDebugVarBinding
{
    std::string funcName;                  ///< Enclosing function name.
    std::string varName;                   ///< Source-level variable or parameter name.
    uint64_t typeId{ 0 };                  ///< 64-bit type ID referencing DebugCompileUnit::types.
    bool isParameter{ false };             ///< True if this binding represents a formal parameter.
    uint32_t paramIndex{ 0 };              ///< 0-based parameter index (valid when isParameter is true).
    EzCore::Debug::DebugSourceLoc declLoc; ///< Source declaration coordinates.
    std::optional<size_t> stackObjId;      ///< Stack frame object ID when stack-allocated.
    std::optional<MirId> vregId;           ///< Pre-SSA root virtual register ID when register-promoted.
};

/**
 * Links a source-level function declaration/definition to its MIR function ID.
 */
struct MirDebugFuncBinding
{
    MirId funcId{ MIRID_INVALID };         ///< MIR function ID in MirBuilderContext.
    std::string name;                      ///< Source-level function name.
    uint64_t returnTypeId{ 0 };            ///< 64-bit return type ID (0 for void).
    EzCore::Debug::DebugSourceLoc declLoc; ///< Source declaration coordinates.
    bool isExternal{ true };               ///< True if the function has external linkage.
};

/**
 * Links a source-level global or static variable to its MIR global variable ID.
 */
struct MirDebugGlobalBinding
{
    MirId gvarId{ MIRID_INVALID };         ///< MIR global variable ID in MirBuilderContext.
    std::string name;                      ///< Global/static symbol name.
    uint64_t typeId{ 0 };                  ///< 64-bit type ID referencing DebugCompileUnit::types.
    EzCore::Debug::DebugSourceLoc declLoc; ///< Source declaration coordinates.
    bool isExternal{ true };               ///< True if the variable has external linkage.
    bool isConstant{ false };              ///< True if the variable is read-only / constant.
};

/**
 * Module-wide debug metadata sidecar stored on MirBuilderContext.
 * Holds the target-independent DebugCompileUnit alongside MIR-to-source bindings
 * populated by frontends and consumed by EmissionEngine after backend lowering.
 */
struct MirDebugMetadata
{
    EzCore::Debug::DebugCompileUnit compileUnit;       ///< Target-independent compile unit container.
    std::vector<MirDebugVarBinding> varBindings;       ///< Parameter and local variable bindings.
    std::vector<MirDebugFuncBinding> funcBindings;     ///< Function bindings.
    std::vector<MirDebugGlobalBinding> globalBindings; ///< Global and static variable bindings.
};

#endif // EZMIR_MIR_DEBUG_METADATA_H
