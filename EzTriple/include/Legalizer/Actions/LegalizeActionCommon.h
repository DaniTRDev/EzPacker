#ifndef EZTRIPLE_LEGALIZE_ACTION_COMMON_H
#define EZTRIPLE_LEGALIZE_ACTION_COMMON_H

#include "EzTripleCommon.h"
#include "HelperClasses/IntrusiveLinkedList.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"

#include <vector>

class MirBuilderContext;
class MirOperand;
class SourceReference;
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

#include "Legalizer/LegalityQuery.h"

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

/**
 * Emits replacement instructions in source order around an anchor instruction.
 *
 * The first emission is scheduled before the anchor; every later emission uses InsertAfter so the
 * sequence stays in order relative to the original instruction, which callers erase separately.
 */
class EmitOrdered
{
  public:
    /**
     * Binds the helper to a builder positioned before @p anchor and to the anchor's source reference.
     */
    EmitOrdered(MirInstructionBuilder &builder, MirInstruction *anchor) :
        m_builder(builder), m_sourceRef(anchor ? anchor->getSourceRef() : nullptr)
    {
    }

    /// Emits an instruction before the anchor in order.
    void emit(MirInstructionOpCode opcode, const std::vector<MirOperand *> &operands)
    {
        m_builder.build(opcode, m_sourceRef, operands);
    }

    /// Callable form so the helper can be passed where an emit callback is expected.
    void operator()(MirInstructionOpCode opcode, const std::vector<MirOperand *> &operands) { emit(opcode, operands); }

  private:
    MirInstructionBuilder &m_builder;     ///< Builder receiving the emitted instructions.
    SourceReference *m_sourceRef;         ///< Source reference attached to every emitted instruction.
};

#endif // EZTRIPLE_LEGALIZE_ACTION_COMMON_H
