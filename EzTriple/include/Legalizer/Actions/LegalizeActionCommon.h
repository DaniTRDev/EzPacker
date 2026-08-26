#ifndef EZTRIPLE_LEGALIZE_ACTION_COMMON_H
#define EZTRIPLE_LEGALIZE_ACTION_COMMON_H

#include "EzTripleCommon.h"
#include "HelperClasses/IntrusiveLinkedList.h"

class MirBuilderContext;
class MirInstruction;
class TargetDesc;

/**
 * Result state of a legalization action.
 */
enum class LegalizationResult : uint8_t
{
    NotModified = 0,
    Legalized,
    Failed
};

/**
 * Predefined legalization action kinds for target machine legalization.
 */
enum class LegalizeAction : uint8_t
{
    Legal = 0,
    WidenScalar,
    NarrowScalar,
    Libcall,
    Custom,
    Bitcast,
    Unsupported
};

/**
 * Context structure passed to legalization action callbacks.
 */
struct LegalizeCtx
{
    MirBuilderContext *m_ctx;
    TargetDesc *m_targetDesc;
    IntrusiveLinkedList<MirInstruction>::iterator m_it;

    LegalizeCtx(MirBuilderContext *ctx, TargetDesc *targetDesc, IntrusiveLinkedList<MirInstruction>::iterator it) :
        m_ctx(ctx), m_targetDesc(targetDesc), m_it(it)
    {
    }
};

#endif // EZTRIPLE_LEGALIZE_ACTION_COMMON_H
