#ifndef EZPACKER_MIRINSTRUCTIONSELECTOR_H
#define EZPACKER_MIRINSTRUCTIONSELECTOR_H

#include "EzTripleCommon.h"

/**
 * Simple wrapper that holds everything needed by the selector.
 */
struct SelectionContext
{
    MirBuilderContext *m_ctx;
    std::pmr::list<MirInstruction *> &m_instrList;
    std::pmr::list<MirInstruction *>::iterator m_it;
};

enum class SelectionResult : uint8_t
{
    AlreadySelected, // The instruction was already selected.
    NoRule,          // The selector does not know how to select this instruction.
    Selected,        // The instruction was selected successfully.
    SelectionError   // There was an error during selection in a particular action.
};

/**
 * This function is able to look upwards/downwards to check the instruction window of the affected instruction (it).
 * This can be used to apply optimisations when selecting an instruction.
 */
using InstructionSelPred = std::function<bool(const SelectionContext &ctx)>;

/**
 * This function is able to insert pre/post instructions within the current selected instruction.
 */
using InstructionSelAction = std::function<SelectionResult(SelectionContext &ctx)>;

struct InstructionSelectionRule
{
    const char *m_name;         // Important for debug purposes.
    InstructionSelAction m_act; // Action that will be run if the predicate evaluates to true.
    InstructionSelPred m_pred;  // If the predicates evaluate to true, the given instruction will be lowered into the
                                // target opcode.
};

class MirInstructionSelector
{
  public:
    /**
     * Creates the selector and the selection rule map using ctx's allocator.
     * @param ctx
     */
    MirInstructionSelector(MirBuilderContext *ctx);

    /**
     * Tries to select the instruction at ctx->m_it. It will try one by one each rule and the first predicate that
     * evaluates to true is the one whose action will be executed.
     * @return
     */
    SelectionResult select(SelectionContext &ctx);

    /**
     * Adds a selection rule for the given opcode.
     * @param opcode
     * @param rule
     */
    void addRule(MirInstructionOpCode opcode, InstructionSelectionRule rule);

  private:
    std::pmr::map<MirInstructionOpCode, std::pmr::vector<InstructionSelectionRule>> m_selectionRules;
};

#endif // EZPACKER_MIRINSTRUCTIONSELECTOR_H
