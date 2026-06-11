#ifndef EZPACKER_AMD64LEGALIZER_H
#define EZPACKER_AMD64LEGALIZER_H

#include "Amd64.h"

class Amd64Legalizer
{
  public:
    /**
     * @brief Creates a fully configured MirLegalizer populated with the AMD64 rule table matrix.
     * @param ctx The builder context used to fetch type tables and register references.
     * @return A unique pointer to a configured, ready-to-use MirLegalizer instance.
     */
    static std::unique_ptr<MirLegalizer> create(MirBuilderContext *ctx);

    const std::vector<size_t> NativeSizes;

  private:
    /**
     * @brief Configures data movement rules:
     * - MOV:
     *  reg{8, 16, 32, 64}, imm{8, 16, 32, 64}.
     *  reg{8, 16, 32, 64}, reg{8, 16, 32, 64}.
     * - LEA:
     *  reg{8, 16, 32, 64}, mem
     */
    void addDataMovement(MirBuilderContext *ctx, MirLegalizer *legalizer);

    /**
     * @brief Configures memory boundary operations and address calculations (e.g., LOAD, STORE, CREATE).
     * - STORE:
     *  mem, reg{8, 16, 32, 64}
     * - LOAD:
     *  reg{8, 16, 32, 64}, mem
     * - CREATE:
     *  reg{8, 16, 32, 64}
     */
    void addMemory(MirBuilderContext *ctx, MirLegalizer *legalizer);

    /**
     * @brief Configures fundamental scalar math instructions (e.g., ADD, ADC, SUB, SBB, MUL, IMUL, DIV, IDIV, REM,
     * NEG).
     * * **Rules Applied:**
     * * `ADD`, `SUB`, `ADC`, `SBB`: **Legal** for primitive native types {i8, i16, i32, i64}.
     * - `i128` types are marked for **Expand** to its high/low components (using `ADD`/`SUB` for the lower 64 bits and
     * `ADC`/`SBB` carry/borrow logic for the upper 64 bits).
     * * `MUL`, `IMUL`: Basic two-operand register-to-register or register-to-immediate forms are **Legal**.
     * * `DIV`, `IDIV`, `REM`: Marked for **Expand** or strict register constraints to account for hardware-enforced
     * implicit physical dependencies on `RAX` and `RDX`.
     * * `NEG`: **Legal** for a single mutable `Register` destination across native scalar types.
     * * **Flags Assertion:** All arithmetic opcodes enforce `SizeMatch` requirements and track `WritesCPUFlags`.
     * * @param legalizer Pointer to the target rule database matrix.
     */
    void addArithmetic(MirBuilderContext *ctx, MirLegalizer *legalizer);

    /**
     * @brief Configures bitwise logical and shifting instructions (e.g., AND, OR, XOR, NOT, SHL, SHR, SAR).
     * * **Rules Applied:**
     * * Logical operations (`AND`, `OR`, `XOR`): Follow strict native width configurations {8, 16, 32, 64} and enforce
     * `SizeMatch`.
     * * Unary logical (`NOT`): **Legal** for native scalar register targets.
     * * MirCat_Bitwise Shifts (`SHL`, `SHR`, `SAR`): Validated to ensure the shift amount operand is either an 8-bit
     * immediate or bound to the implicit `CL` physical register. Variable shifts utilizing a non-CL virtual register
     * are marked for **Expand** to inject a register-copy step. `SAR` enforces `TreatAsSigned`.
     * * @param legalizer Pointer to the target rule database matrix.
     */
    void addBitwise(MirBuilderContext *ctx, MirLegalizer *legalizer);

    /**
     * @brief Configures condition testing and flags management instructions (e.g., CMP, TEST).
     * * **Rules Applied:**
     * * `CMP`, `TEST`: **Legal** for matching scalar sizes. `TEST` implies commutativity.
     * - Compares between two memory locations are marked for **Expand** to pass one operand through a scratch register.
     * * If `SETcc` logic is required to evaluate conditions into boolean destinations, targets smaller than 8 bits are
     * widened to an `i8` byte-sized register representation.
     * * @param legalizer Pointer to the target rule database matrix.
     */
    void addCompare(MirBuilderContext *ctx, MirLegalizer *legalizer);

    /**
     * @brief Configures jump statements and block execution routing (e.g., JMP, JE, JNE, JG, JGE, JL, JLE, JA, JB,
     * CALL, RET).
     * * **Rules Applied:**
     * * Direct Control Flow: Unconditional `JMP` and all conditional branches (`JE`, `JNE`, `JG`, `JGE`, `JL`, `JLE`,
     * `JA`, `JB`) targeting valid `Reference` blocks are marked **Legal**. Conditional branches assert `ReadsCPUFlags`.
     * * `CALL`: Subroutine routing targeting a `Reference` label or an indirect 64-bit `Register` is **Legal**.
     * 32-bit indirect addresses are automatically intercepted and zero-extended.
     * * `RET`: **Legal** for returning standard values or running empty registers. Implements `IsTerminator`.
     * * @param legalizer Pointer to the target rule database matrix.
     */
    void addControlFlow(MirBuilderContext *ctx, MirLegalizer *legalizer);

    /**
     * @brief Configures type transformations, sign/zero extensions, and casting (e.g., TRUNC, ZEXT, SEXT, BITCAST).
     * * **Rules Applied:**
     * * `ZEXT`, `SEXT`: Extensions moving up from native source bounds {i8, i16, i32} to larger native targets are
     * **Legal** * and lower to hardware primitives (`MOVZX`/`MOVSX`). Non-native bit-widths (e.g., `i24`) trigger
     * **Expand** mask-and-shift sequences.
     * * `TRUNC`: Shrinks a source down into a narrower destination (`DestSmaller`). Marked **Legal** if the underlying
     * target sub-register is directly addressable.
     * * `BITCAST`: Reinterprets bits without conversion. Marked **Legal** if it satisfies structural `SizeMatch`
     * properties.
     * * @param legalizer Pointer to the target rule database matrix.
     */
    void addCasting(MirBuilderContext *ctx, MirLegalizer *legalizer);

    /**
     * @brief Configures low-level system calls, barriers, and environmental actions (e.g., SYSCALL, NOP, HALT).
     * * **Rules Applied:**
     * * `SYSCALL`: Rigid environmental hook. Bypasses standard virtual registers, enforcing hardcoded physical ABI
     * boundaries.
     * * `NOP`: Always **Legal**. Completely transparent operation with no side effects.
     * * `HALT`: Terminates immediate execution paths. Marked with `IsTerminator` and `HasSideEffect`.
     * * @param legalizer Pointer to the target rule database matrix.
     */
    void addSystem(MirBuilderContext *ctx, MirLegalizer *legalizer);
};

#endif // EZPACKER_AMD64LEGALIZER_H
