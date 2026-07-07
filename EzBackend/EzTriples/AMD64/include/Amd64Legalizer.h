#ifndef EZPACKER_AMD64LEGALIZER_H
#define EZPACKER_AMD64LEGALIZER_H

#include "Amd64.h"
#include "Descriptors/Amd64TargetDesc.h"
#include "Type/MirType.h"

class Amd64Legalizer
{
  public:
    /**
     * @brief Creates a fully configured MirLegalizer populated with the AMD64 rule table matrix.
     * @param targetDesc The target description of the AMD64 architecture.
     * @param ctx The builder context used to fetch type tables and register references.
     * @return A unique pointer to a configured, ready-to-use MirLegalizer instance.
     */
    static std::unique_ptr<MirLegalizer> create(Amd64TargetDesc *targetDesc, MirBuilderContext *ctx);

    /**
     * @brief Returns the native sizes allowed for this architecture.
     * @param ctx The builder context used to fetch the type system references.
     * @return A vector of MirType* containing the natively supported scalar types (i8, i16, i32, i64).
     */
    const std::vector<MirType*> getNativeSizes(MirBuilderContext *ctx) const;

  private:
    /**
     * @brief Configures data movement rules:
     * - MOV:
     * reg{8, 16, 32, 64}, imm{8, 16, 32, 64}.
     * reg{8, 16, 32, 64}, reg{8, 16, 32, 64}.
     * - LEA:
     * reg{8, 16, 32, 64}, mem
     */
    void addDataMovement(MirBuilderContext *ctx, MirLegalizer *legalizer);

    /**
     * @brief Configures memory boundary operations and address calculations:
     * - STORE:
     * mem, reg{8, 16, 32, 64}
     * - LOAD:
     * reg{8, 16, 32, 64}, mem
     * - CREATE:
     * reg{64}
     */
    void addMemory(MirBuilderContext *ctx, MirLegalizer *legalizer);

    /**
     * @brief Configures fundamental scalar math instructions:
     * - ADD, ADC, SUB, SBB, MUL, IMUL, DIV, IDIV, REM:
     * reg{8, 16, 32, 64}, reg{8, 16, 32, 64}
     * reg{8, 16, 32, 64}, imm{8, 16, 32, 64}
     * - NEG:
     * reg{8, 16, 32, 64}
     */
    void addArithmetic(MirBuilderContext *ctx, MirLegalizer *legalizer);

    /**
     * @brief Configures bitwise logical and shifting instructions:
     * - AND, OR, XOR, SHL, SHR, SAR:
     * reg{8, 16, 32, 64}, reg{8, 16, 32, 64}
     * reg{8, 16, 32, 64}, imm{8, 16, 32, 64}
     * - NOT:
     * reg{8, 16, 32, 64}
     */
    void addBitwise(MirBuilderContext *ctx, MirLegalizer *legalizer);

    /**
     * @brief Configures condition testing and flags management instructions:
     * - CMP, TEST:
     * reg{8, 16, 32, 64}, reg{8, 16, 32, 64}
     * reg{8, 16, 32, 64}, imm{8, 16, 32, 64}
     */
    void addCompare(MirBuilderContext *ctx, MirLegalizer *legalizer);

    /**
     * @brief Configures jump statements and block execution routing:
     * - JMP, JE, JNE, JG, JGE, JL, JLE, JA, JB:
     * ref
     * - CALL:
     * ref
     * reg{64}
     * - RET:
     * reg{8, 16, 32, 64}
     * imm{8, 16, 32, 64}
     */
    void addControlFlow(MirBuilderContext *ctx, MirLegalizer *legalizer);

    /**
     * @brief Configures type transformations, sign/zero extensions, and casting:
     * - TRUNC:
     * reg{8, 16, 32}, reg{16, 32, 64} (where dest < src)
     * - ZEXT, SEXT:
     * reg{16, 32, 64}, reg{8, 16, 32} (where dest > src)
     * - BITCAST:
     * reg{8, 16, 32, 64}, reg{8, 16, 32, 64} (where dest == src)
     */
    void addCasting(MirBuilderContext *ctx, MirLegalizer *legalizer);

    /**
     * @brief Configures low-level system calls, barriers, and environmental actions:
     * - SYSCALL:
     * reg{64}
     * - NOP, HALT:
     * (no operands)
     */
    void addSystem(MirBuilderContext *ctx, MirLegalizer *legalizer);
};

#endif // EZPACKER_AMD64LEGALIZER_H
