#ifndef EZTRIPLE_MIR_INSTRUCTION_SELECTOR_H
#define EZTRIPLE_MIR_INSTRUCTION_SELECTOR_H

#include "InstructionSelector/MirAddressingModeMatcher.h"
#include <list>
#include <vector>

class MirBlock;
class MirBuilderContext;
class MirFunction;
class MirInstruction;
class TargetDesc;
class MirOperand;

/**
 * Abstract interface for target instruction selection.
 * Synthesized target selectors implement pattern matching decision trees to replace generic MIR instructions
 * with hardware instructions and register class constraints.
 */
class MirInstructionSelector
{
  public:
    explicit MirInstructionSelector(TargetDesc *targetDesc = nullptr) : m_targetDesc(targetDesc) {}
    virtual ~MirInstructionSelector() = default;

    /// Returns the target descriptor currently bound to this selector.
    TargetDesc *getTargetDesc() const noexcept { return m_targetDesc; }

    /// Rebinds the selector to a different target descriptor.
    void setTargetDesc(TargetDesc *targetDesc) noexcept { m_targetDesc = targetDesc; }

    /// Records the function whose blocks are currently being processed.
    void setCurrentFunction(MirFunction *func) noexcept { m_currentFunction = func; }

    /// Records the block whose instructions are currently being processed.
    void setCurrentBlock(MirBlock *block) noexcept { m_currentBlock = block; }

    /// Returns the function being processed, used to resolve register uses and definitions.
    MirFunction *getCurrentFunction() const noexcept { return m_currentFunction; }

    /// Returns the block being processed, used to locate insertion points.
    MirBlock *getCurrentBlock() const noexcept { return m_currentBlock; }

    /**
     * Selects and replaces a generic instruction with target hardware instructions.
     * Active builder context.
     * Instruction to select and replace.
     * True if the instruction was recognized and successfully transformed.
     */
    virtual bool select(MirBuilderContext *ctx, MirInstruction *inst) = 0;

    /**
     * Runs instruction selection across all blocks in a function.
     */
    virtual bool selectFunction(MirBuilderContext *ctx, MirFunction *func);

    /**
     * Runs instruction selection across all instructions in a basic block using Bottom-Up Maximal Munch.
     */
    virtual bool selectBlock(MirBuilderContext *ctx, MirBlock *block);

    /**
     * Automatically assigns target register classes to virtual register operands according to
     * MirTargetInstructionDesc specifications.
     */
    void assignRegisterClasses(MirInstruction *inst);

    /**
     * Attempts to fold an address computation tree into a hardware addressing mode using the target's matcher.
     */
    bool foldAddressingMode(MirBuilderContext *ctx,
                            MirOperand *addrOp,
                            MatchedAddressingMode &outMode,
                            MirInstruction *rootInst = nullptr);

    /**
     * Erases folded child instructions from their parent basic block.
     */
    void eraseFoldedInstructions(const std::vector<MirInstruction *> &folded);

    /**
     * Checks if a virtual register has exactly one use in the owning function.
     */
    bool hasOneUse(class MirRegister *reg) const;

    /**
     * Checks if there are no memory stores, calls, or unmodeled side effects between two instructions.
     */
    bool noInterveningStore(MirInstruction *from, MirInstruction *to) const;

    /**
     * Returns the defining instruction for a virtual register in the active function.
     */
    MirInstruction *getDefiningInstruction(class MirRegister *reg) const;

    /**
     * Returns the defining instruction for a virtual register while explicitly supplying the
     * builder context, allowing lookups outside the selector's cached function state.
     */
    MirInstruction *getDefiningInstruction(MirBuilderContext *ctx, class MirRegister *reg) const;

  protected:
    TargetDesc *m_targetDesc{ nullptr };       ///< Target the selector is generating code for.
    MirBlock *m_currentBlock{ nullptr };       ///< Block currently being visited.
    MirFunction *m_currentFunction{ nullptr }; ///< Function currently being visited.
};

#endif // EZTRIPLE_MIR_INSTRUCTION_SELECTOR_H
