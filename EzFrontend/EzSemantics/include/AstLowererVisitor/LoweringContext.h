#ifndef EZPACKER_LOWERINGCONTEXT_H
#define EZPACKER_LOWERINGCONTEXT_H

#include "EzSemanticsCommon.h"

/**
 * Struct to hold the context of a loop during lowering. This is useful for handling break and continue statements, as
 * it provides the necessary targets for these statements to jump to.
 */
struct LoopContext
{
    MirBlock *m_breakTarget{ nullptr };
    MirBlock *m_continueTarget{ nullptr };
};

class LoweringContext
{
  public:
    /**
     * Creates the lowering context with all the required dependencies.
     * @param semanticCtx The semantic context.
     * @param emitter The MIR emitter.
     * @param emitterContext The MIR emitter context.
     * @param globalDataEmitter The global data emitter.
     */
    LoweringContext(const std::shared_ptr<struct BasicSemanticContext> &semanticCtx,
                    const std::shared_ptr<struct MirEmitter> &emitter,
                    const std::shared_ptr<struct MirEmitterContext> &emitterContext,
                    const std::shared_ptr<struct MirGlobalDataEmitter> &globalDataEmitter);

    /**
     * Returns the pointer to the visitor that owns (is using) this context.
     * @return AstLowererVisitor *
     */
    class AstLowererVisitor *getOwnerLowererVisitor() const;

    /**
     * Returns true if this context has at least 1 block in the block stack.
     * @return bool
     */
    bool hasBlocks() const;

    /**
     * Returns true if this lowerer has appended instructions to the instruction stack.
     * @return bool
     */
    bool hasInstructions() const;

    /**
     * Returns true if this lowerer has appended operands to the operand stack.
     * @return bool
     */
    bool hasOperands() const;

    /**
     * Pops a block (if any) from the block stack and returns it. If block stack is empty, an exception is thrown.
     * @return MirBlock *
     */
    MirBlock *popBlock();

    /**
     * Pops an instruction (if any) from the instruction stack and returns it. If instruction stack is empty, an
     * exception is thrown.
     * @return MirInstruction *
     */
    MirInstruction *popInstruction();

    /**
     * Pops an operand (if any) from the operand stack and returns it. If operand stack is empty, an exception is
     * thrown.
     * @return MirOperand
     */
    MirOperand popOperand();

    /**
     * Enters a loop context by pushing the provided loop context onto the block stack. This is useful for handling
     * break and continue statements, as it provides the necessary targets for these statements to jump to.
     * @param loopContext
     */
    void enterLoop(const LoopContext &loopContext);

    /**
     * Exits the current loop context by popping it from the block stack. This should be called when exiting a loop
     * construct to ensure that the correct context is maintained for nested loops.
     */
    void exitLoop();

    /**
     * Pushes a new block to the block stack.
     * @param block
     */
    void pushBlock(MirBlock *block);

    /**
     * Pushes an operand onto the operand stack used during lowering. This stack is useful for handling expressions and
     * temporary values. Callers can use this method to pass operands up the tree so parent nodes (like instructions or
     * expressions) can use them.
     * @param operand
     */
    void pushOperand(MirOperand operand);

    /**
     * Pushes an instruction to the instruction stack.
     * @param instruction
     */
    void pushInstruction(MirInstruction *instruction);

    /**
     * Sets the owner visitor of this node.
     * @param ownerVisitor
     */
    void setOwnerVisitor(class AstLowererVisitor *ownerVisitor);

    /**
     * Returns the current loop context. If there are no loop contexts in the stack, an exception is thrown.
     * @return const LoopContext &
     */
    const LoopContext &getCurrentLoopContext() const;

    /**
     * Returns the semantic context linked to this context.
     * @return const std::shared_ptr<struct BasicSemanticContext> &
     */
    const std::shared_ptr<struct BasicSemanticContext> &getSemanticContext() const;

    /**
     * Returns the emitter linked to this context.
     * @return const std::shared_ptr<struct MirEmitter> &
     */
    const std::shared_ptr<struct MirEmitter> &getEmitter() const;

    /**
     * Returns the emitter context for the emitters of this context.
     * @return const std::shared_ptr<struct MirEmitter> &
     */
    const std::shared_ptr<struct MirEmitterContext> &getEmitterContext() const;

    /**
     * Returns the global data emitter linked to this context.
     * @return const std::shared_ptr<struct MirGlobalDataEmitter> &
     */
    const std::shared_ptr<struct MirGlobalDataEmitter> &getGlobalDataEmitter() const;

    /**
     * Returns the block stack used during lowering.
     * @return const std::stack<MirBlock *> &
     */
    const std::stack<MirBlock *> &getBlockStack() const;

    /**
     * Returns the operand stack used during lowering.
     * @return const std::stack<MirInstruction *> &
     */
    const std::stack<MirInstruction *> &getInstructionStack() const;

    /**
     * Returns the operand stack used during lowering. This stack is useful for handling expressions and
     * temporary values.
     * @return const std::stack<MirOperand> &
     */
    const std::stack<MirOperand> &getOperandStack() const;

  private:
    class AstLowererVisitor *m_ownerVisitor; // Visitor that is using this context.
    std::shared_ptr<struct BasicSemanticContext> m_semanticCtx;
    std::shared_ptr<struct MirEmitter> m_emitter;
    std::shared_ptr<struct MirEmitterContext> m_emitterContext;
    std::shared_ptr<struct MirGlobalDataEmitter> m_globalDataEmitter;
    std::stack<LoopContext>
            m_loopContextStack;          // Stack to manage nested loop contexts for break and continue statements.
    std::stack<MirBlock *> m_blockStack; // Used to transfer blocks between lowerers.
    std::stack<MirInstruction *> m_instructionStack;
    std::stack<MirOperand>
            m_operandStack; // Stack to hold operands during lowering, useful for expressions and temporary values.
};

#endif // EZPACKER_LOWERINGCONTEXT_H
