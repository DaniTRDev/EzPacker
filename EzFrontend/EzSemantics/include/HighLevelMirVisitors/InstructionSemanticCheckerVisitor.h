#ifndef EZPACKER_INSTRUCTIONSEMANTICCHECKERVISITOR_H
#define EZPACKER_INSTRUCTIONSEMANTICCHECKERVISITOR_H

#include "EzSemanticsCommon.h"
#include "BasicSemanticContext.h"
#include "HighLevelMirVisitor.h"
#include "HighLevelMir/HighLevelMirModule.h"

class InstructionSemanticCheckerVisitor : public HighLevelMirVisitor
{
  public:
    /**
     * Visits the given MIR module.
     * @param module
     * @return bool
     */
    bool visit(const class HighLevelMirModule &module) override;

    /**
     * Visits the given MIR block.
     * @param block
     * @return bool
     */
    bool visit(const class HighLevelMirBlock &block) override;

    /**
     * Visits the given MIR instruction.
     * @param instruction
     * @return bool
     */
    bool visit(const class HighLevelMirInstruction &instruction) override;

  private:
    /**
     * Visits the given block (scope body) by traversing its instructions. Returns true if succeeded.
     * @param block
     * @return bool
     */
    bool visitScopeBody(const class HighLevelMirBlock &block);

    /**
     * Checks if the given instruction has the expected operand types. Returns true if succeeded.
     * @param instruction
     * @param instrFlags
     * @param operands
     * @return bool
     */
    bool expectOperandsType(const HighLevelMirInstruction &instruction,
                            uint32_t instrFlags,
                            const std::vector<HighLevelMirInstructionOperand> &operands);
};

#endif // EZPACKER_INSTRUCTIONSEMANTICCHECKERVISITOR_H
