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
    std::string funcName;
    std::string varName;
    uint64_t typeId{ 0 };
    bool isParameter{ false };
    uint32_t paramIndex{ 0 };
    EzCore::Debug::DebugSourceLoc declLoc;
    std::optional<size_t> stackObjId;
    std::optional<MirId> vregId;
};

/**
 * Links a source-level function declaration/definition to its MIR function ID.
 */
struct MirDebugFuncBinding
{
    MirId funcId{ MIRID_INVALID };
    std::string name;
    uint64_t returnTypeId{ 0 };
    EzCore::Debug::DebugSourceLoc declLoc;
    bool isExternal{ true };
};

/**
 * Links a source-level global or static variable to its MIR global variable ID.
 */
struct MirDebugGlobalBinding
{
    MirId gvarId{ MIRID_INVALID };
    std::string name;
    uint64_t typeId{ 0 };
    EzCore::Debug::DebugSourceLoc declLoc;
    bool isExternal{ true };
    bool isConstant{ false };
};

/**
 * Module-wide debug metadata sidecar stored on MirBuilderContext.
 * Holds the target-independent DebugCompileUnit alongside MIR-to-source bindings
 * populated by frontends and consumed by EmissionEngine after backend lowering.
 */
struct MirDebugMetadata
{
    EzCore::Debug::DebugCompileUnit compileUnit;
    std::vector<MirDebugVarBinding> varBindings;
    std::vector<MirDebugFuncBinding> funcBindings;
    std::vector<MirDebugGlobalBinding> globalBindings;
};

#endif // EZMIR_MIR_DEBUG_METADATA_H
