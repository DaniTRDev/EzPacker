#ifndef EZTRIPLE_MIR_LEGALIZER_H
#define EZTRIPLE_MIR_LEGALIZER_H

#include "EzTripleCommon.h"
#include "HelperClasses/IntrusiveLinkedList.h"
#include "Legalizer/Actions/LegalizeActionCommon.h"

#include <string_view>

class MirBlock;
class MirBuilderContext;
class MirFunction;
class MirInstruction;
class MirType;
class TargetDesc;

/**
 * Base abstract class / driver for target machine legalization.
 * Evaluates generic MIR instructions against the target's legality matrix, performing
 * scalar widening, narrowing, libcall substitution, or delegating to rewrite rules.
 */
class MirLegalizer
{
  public:
    MirLegalizer(MirBuilderContext *ctx, TargetDesc *targetDesc);
    virtual ~MirLegalizer() = default;

    /**
     * Legalizes all basic blocks and instructions in the specified function.
     */
    virtual bool legalizeFunction(MirFunction *func);

    /**
     * Legalizes all instructions within a single basic block.
     */
    virtual bool legalizeBlock(MirBlock *block);

    /**
     * Builds a LegalityQuery capturing all operands, types, and flags for an instruction.
     */
    virtual LegalityQuery buildQuery(MirInstruction *inst);

    /**
     * Executes the legalization action specified in the legality response.
     */
    virtual LegalizationResult executeAction(const LegalityResponse &response, LegalizeCtx &ctx, MirInstruction *inst);

  protected:
    MirBuilderContext *m_ctx; ///< Shared builder context used to create rewrites.
    TargetDesc *m_targetDesc; ///< Target providing legality info and legalization actions.
};

#endif // EZTRIPLE_MIR_LEGALIZER_H
