#ifndef EZPACKER_X86_64_CODE_EMITTER_H
#define EZPACKER_X86_64_CODE_EMITTER_H

#include "EzCodeEmitterCommon.h"
#include "GenericCodeEmitter.h"
#include "X86_64/Encoding/X86_64EncodingDesc.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "Operand/MirOperands.h"
#include <functional>
#include <vector>

namespace EzCodeEmitter::X86_64
{

/**
 * Resolves the declarative EncodingDesc for a target instruction descriptor.
 *
 * Supplied by the target descriptor (which owns the generated encoding table), so the
 * emitter stays independent from generated code and from EzTriple.
 */
using EncodingResolver = std::function<const EncodingDesc *(MirTargetInstructionDesc *)>;

/**
 * Concrete x86-64 Machine Code Emitter implementing GenericCodeEmitter.
 * Encodes target-lowered machine instructions into binary sections by interpreting the
 * declarative EncodingDesc supplied for each instruction descriptor.
 */
class X86_64CodeEmitter : public GenericCodeEmitter
{
  public:
    X86_64CodeEmitter() = default;
    ~X86_64CodeEmitter() override = default;

    /**
     * Starts a function, creating and binding an entry label of the given name in .text.
     */
    void beginFunction(CodeEmitterContext *ctx, std::string_view name) override;

    /**
     * Starts a function described by a MIR function, remembering it for stack-frame
     * resolution and deriving the entry label name from it.
     */
    void beginFunction(CodeEmitterContext *ctx, MirFunction *func) override;

    /**
     * Materializes and binds the label identified by labelId at the current position.
     */
    void bindLabel(MirId labelId) override;

    /**
     * Ends the current function, flushing its labels and relocations to the context.
     */
    void endFunction(CodeEmitterContext *ctx) override;

    /**
     * Ends the current function and associates the flushed state with func.
     */
    void endFunction(CodeEmitterContext *ctx, MirFunction *func) override;

    /**
     * Encodes and emits a single lowered target instruction via the table-driven encoder.
     */
    void emitInst(MirTargetInstructionDesc *desc, std::span<MirOperand *> operands) override;

    /**
     * Installs the table-driven encoding resolver. Emission is skipped when no resolver
     * is installed or when it cannot resolve the instruction's encoding.
     */
    void setEncodingResolver(EncodingResolver resolver) { m_encodingResolver = std::move(resolver); }

  private:
    /**
     * Maps a physical register operand to its hardware encoding. The returned value is in
     * the x86-64 0..31 convention, where values >= 16 identify an FPR/XMM register.
     */
    uint8_t mapRegister(MirRegister *reg) const;

    /**
     * Resolves and emits an instruction via the table-driven encoder. Returns false when
     * no encoding is available, allowing the caller to bail out.
     */
    bool tryEmitTableDriven(MirTargetInstructionDesc *desc, std::span<MirOperand *> operands);

    /**
     * Converts EzMir operands into the target-neutral representation consumed by the
     * table-driven encoder, resolving stack slots and global references to memory.
     */
    bool buildResolvedOperands(const EncodingDesc &enc,
                               std::span<MirOperand *> operands,
                               std::vector<ResolvedOperand> &resolved) const;

  private:
    CodeEmitterContext *m_ctx{ nullptr };  ///< Active emission context (labels/relocations/sections).
    MirFunction *m_currentFunc{ nullptr }; ///< Function currently being emitted, if any.
    EncodingResolver m_encodingResolver;   ///< Resolves instruction descriptors to encoding descriptors.
};

} // namespace EzCodeEmitter::X86_64

#endif // EZPACKER_X86_64_CODE_EMITTER_H
