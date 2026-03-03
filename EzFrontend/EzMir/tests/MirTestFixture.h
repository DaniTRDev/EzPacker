#ifndef EZPACKER_MIRTESTFIXTURE_H
#define EZPACKER_MIRTESTFIXTURE_H

#include <gtest/gtest.h>
#include "EzMir.h"

class MirTestFixture : public ::testing::Test
{
  public:
    void SetUp() override;
    void TearDown() override;

    // ──────────────────────────────────────────────────────────────
    //  Block helpers
    // ──────────────────────────────────────────────────────────────

    /** Creates a block via the context. */
    MirBlock *createBlock();

    /** Creates a block AND binds the context to it. */
    MirBlock *createAndBindBlock();

    // ──────────────────────────────────────────────────────────────
    //  Instruction helpers
    // ──────────────────────────────────────────────────────────────

    /** Returns the i-th instruction of a block (nullptr if out of range). */
    MirInstruction *getInstruction(MirBlock *block, size_t index);

    /** Returns the number of instructions in a block. */
    size_t getInstructionCount(MirBlock *block);

    // ──────────────────────────────────────────────────────────────
    //  Operand helpers
    // ──────────────────────────────────────────────────────────────

    /** Returns the i-th operand of an instruction (nullptr if out of range). */
    MirOperand *getOperand(MirInstruction *instr, size_t index);

    /** Returns the number of operands of an instruction. */
    size_t getOperandCount(MirInstruction *instr);

    // ──────────────────────────────────────────────────────────────
    //  Assertion helpers
    // ──────────────────────────────────────────────────────────────

    /** Asserts that a block has exactly `expected` instructions. */
    void expectInstructionCount(MirBlock *block, size_t expected);

    /** Asserts that the i-th instruction of a block has the expected opcode. Returns the instruction. */
    MirInstruction *expectOpcode(MirBlock *block, size_t instrIdx, MirInstructionOpCode expected);

    /** Asserts that an instruction has the expected operand count. */
    void expectOperandCount(MirInstruction *instr, size_t expected);

    /** Asserts that the i-th operand of an instruction is of the given type. Returns the operand. */
    MirOperand *expectOperandType(MirInstruction *instr, size_t opIdx, MirOperandType expected);

  protected:
    std::shared_ptr<ErrorCollector> m_errorCollector;
    std::shared_ptr<SourceManager> m_sourceManager;
    std::shared_ptr<MirEmitterContext> m_context;
    std::shared_ptr<MirEmitter> m_emitter;
    std::shared_ptr<MirGlobalDataEmitter> m_globalDataEmitter;
};

#endif // EZPACKER_MIRTESTFIXTURE_H
