/**
 * @file AstLoweringContext.h
 * @brief Shared mutable state used while lowering Ez AST nodes to MIR.
 *
 * `AstLoweringContext` is the coordination object passed to all lowerers. It
 * keeps temporary lowering artefacts on stacks, remembers how semantic symbols
 * map to MIR IDs, exposes the MIR emitter services, and tracks the active loop
 * nesting needed by `break` / `continue` lowering.
 *
 * The object does not own semantic symbols or AST nodes. It stores shared
 * pointers to the semantic context and MIR services supplied by the caller.
 */
#ifndef EZPACKER_ASTLOWERINGCONTEXT_H
#define EZPACKER_ASTLOWERINGCONTEXT_H

#include "EzSemanticsCommon.h"
#include "Scope/Symbol.h"

/**
 * Jump targets for the innermost loop currently being lowered.
 */
struct LoopContext
{
    MirBlock *m_breakTarget{ nullptr };
    MirBlock *m_continueTarget{ nullptr };
};

class AstLoweringContext
{
  public:
    /**
     * Creates the lowering context with all MIR and semantic dependencies
     * required by the public lowerers.
     */
    AstLoweringContext(const std::shared_ptr<struct BasicSemanticContext> &semanticCtx,
                    const std::shared_ptr<struct MirEmitter> &emitter,
                    const std::shared_ptr<struct MirEmitterContext> &emitterContext);

    /**
     * Returns the `AstLowererVisitor` currently using this context, if one has
     * been registered.
     */
    class AstLowererVisitor *getOwnerLowererVisitor() const;

    /**
     * Returns whether at least one MIR block is waiting on the block stack.
     */
    bool hasBlocks() const;

    /**
     * Returns whether at least one MIR instruction is waiting on the
     * instruction stack.
     */
    bool hasInstructions() const;

    /**
     * Returns whether at least one MIR operand is waiting on the operand
     * stack.
     */
    bool hasOperands() const;

    /**
     * Returns whether the given semantic symbol has already been linked to a
     * MIR ID.
     */
    bool isSymbolLinkedToMir(Symbol *symbol) const;

    /**
     * Associates one semantic symbol with a MIR ID.
     *
     * @return `false` if the symbol was already linked.
     */
    bool linkSymbolToMirId(Symbol *sym, size_t mirId);

    /**
     * Associates one semantic type name with a MIR type ID.
     *
     * This lets later lowering steps reuse the same MIR type object for the
     * same semantic type.
     *
     * @return `false` if the semantic type name was already linked.
     */
    bool linkTypeNameToMirTypeId(Type *semanticType, size_t mirTypeId);

    /**
     * Pops and returns the top MIR block from the block stack.
     *
     * Throws if the stack is empty.
     */
    MirBlock *popBlock();

    /**
     * Returns the MIR ID linked to the given symbol, or `MIRID_INVALID` when
     * no mapping exists.
     */
    MirId getMirIdOfSymbol(Symbol *sym) const;

    /**
     * Pops and returns the top MIR instruction from the instruction stack.
     *
     * Throws if the stack is empty.
     */
    MirInstruction *popInstruction();

    /**
     * Pops and returns the top MIR operand from the operand stack.
     *
     * Throws if the stack is empty.
     */
    MirOperand popOperand();

    /**
     * Returns a MIR type corresponding to the provided semantic type.
     *
     * The context caches the name-to-MIR mapping, so repeated requests for the
     * same semantic type reuse the same MIR type object.
     */
    MirType *createMirTypeFromSemanticType(Type *semanticType);

    /**
     * Pushes a loop context used by `break` and `continue` lowering.
     */
    void enterLoop(const LoopContext &loopContext);

    /**
     * Pushes a block used by `break` lowering.
     */
    void enterSwitch(MirBlock *breakBlock);

    /**
     * Pops the current loop context.
     *
     * Throws if no loop context is active.
     */
    void exitLoop();

    /**
     * Pops the current switch.
     *
     * Throws if no switch is active.
     */
    void exitSwitch();

    /**
     * Pushes a MIR block onto the block stack.
     */
    void pushBlock(MirBlock *block);

    /**
     * Pushes a MIR operand onto the operand stack.
     */
    void pushOperand(MirOperand operand);

    /**
     * Pushes a MIR instruction onto the instruction stack.
     */
    void pushInstruction(MirInstruction *instruction);

    /**
     * Registers the lowering visitor currently using this context.
     */
    void setOwnerVisitor(class AstLowererVisitor *ownerVisitor);

    /**
     * Returns the active innermost loop context.
     *
     * Throws if no loop context is active.
     */
    const LoopContext &getCurrentLoopContext() const;

    /**
     * Returns the semantic context shared with lowering.
     */
    const std::shared_ptr<struct BasicSemanticContext> &getSemanticContext() const;

    /**
     * Returns the MIR emitter used to create blocks, instructions and other
     * MIR artefacts.
     */
    const std::shared_ptr<struct MirEmitter> &getEmitter() const;

    /**
     * Returns the MIR emitter context used to cache and query MIR types and
     * other emitter-managed state.
     */
    const std::shared_ptr<struct MirEmitterContext> &getEmitterContext() const;

    /**
     * Returns the current block stack.
     */
    const std::stack<MirBlock *> &getBlockStack() const;

    /**
     * Returns the current instruction stack.
     */
    const std::stack<MirInstruction *> &getInstructionStack() const;

    /**
     * Returns the current operand stack.
     */
    const std::stack<MirOperand> &getOperandStack() const;

  private:
    class AstLowererVisitor *m_ownerVisitor;   // Visitor that is using this context.
    std::map<size_t, size_t> m_symbolToMirMap; // Map that links a symbol with its corresponding MIR ID.
    std::map<std::string_view, size_t> m_typeNameToMirTypeIdMap; // Links a semantic type name to its MIR type ID.
    std::shared_ptr<struct BasicSemanticContext> m_semanticCtx;
    std::shared_ptr<struct MirEmitter> m_emitter;
    std::shared_ptr<struct MirEmitterContext> m_emitterContext;
    std::stack<LoopContext> m_loopContextStack;  // Used to know how to handle break/continue in nested loops.
    std::stack<MirBlock *> m_switchContextStack; // Used to know how to handle break in nested switches.
    std::stack<MirBlock *> m_blockStack;         // Used to transfer blocks between lowerers.
    std::stack<MirInstruction *> m_instructionStack;
    std::stack<MirOperand>
            m_operandStack; // Stack to hold operands during lowering, useful for expressions and temporary values.
};

#endif // EZPACKER_ASTLOWERINGCONTEXT_H
