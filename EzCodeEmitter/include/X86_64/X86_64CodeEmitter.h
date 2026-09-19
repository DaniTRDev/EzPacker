#ifndef EZPACKER_X86_64_CODE_EMITTER_H
#define EZPACKER_X86_64_CODE_EMITTER_H

#include "EzCodeEmitterCommon.h"
#include "GenericCodeEmitter.h"
#include "X86_64/X86_64Encoding.h"
#include "TableGen/EncodingDesc.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "Operand/MirOperands.h"
#include <functional>
#include <vector>

namespace EzCodeEmitter::X86_64
{

/**
 * Physical register mapping function translating MirRegisterRef / physical ID into X86_64::Reg.
 */
using RegMapper = std::function<Reg(size_t physId)>;

/**
 * Resolves the declarative EncodingDesc for a target instruction descriptor.
 *
 * Supplied by the target descriptor (which owns the generated encoding table), so the
 * emitter stays independent from generated code and from EzTriple.
 */
using EncodingResolver = std::function<const TableGen::EncodingDesc *(MirTargetInstructionDesc *)>;

/**
 * Concrete x86-64 Machine Code Emitter implementing GenericCodeEmitter.
 * Encodes target-lowered machine instructions directly into binary sections
 * using exact Intel 64 machine instruction encoding tables.
 */
class X86_64CodeEmitter : public GenericCodeEmitter
{
  public:
    explicit X86_64CodeEmitter(RegMapper regMapper = nullptr);
    ~X86_64CodeEmitter() override = default;

    void beginFunction(CodeEmitterContext *ctx, std::string_view name) override;
    void beginFunction(CodeEmitterContext *ctx, MirFunction *func);
    void bindLabel(MirId labelId) override;
    void endFunction(CodeEmitterContext *ctx) override;
    void endFunction(CodeEmitterContext *ctx, MirFunction *func);
    void emitInst(MirTargetInstructionDesc *desc, std::span<MirOperand *> operands) override;

    /**
     * Sets a custom physical register ID mapper.
     */
    void setRegMapper(RegMapper mapper) { m_regMapper = std::move(mapper); }

    /**
     * Installs the table-driven encoding resolver. When unset (or when it returns a
     * descriptor without an encoding), emission falls back to the built-in encoder.
     */
    void setEncodingResolver(EncodingResolver resolver) { m_encodingResolver = std::move(resolver); }

  private:
    Reg mapRegister(MirRegister *reg) const;
    MemoryOperand mapMemory(MirMemory *mem) const;
    MemoryOperand mapOperandToMemory(MirOperand *op) const;

    bool tryEmitTableDriven(MirTargetInstructionDesc *desc, std::span<MirOperand *> operands);
    bool buildResolvedOperands(const TableGen::EncodingDesc &enc,
                               std::span<MirOperand *> operands,
                               std::vector<TableGen::ResolvedOperand> &resolved) const;

  private:
    CodeEmitterContext *m_ctx{ nullptr };
    MirFunction *m_currentFunc{ nullptr };
    RegMapper m_regMapper;
    EncodingResolver m_encodingResolver;
};

} // namespace EzCodeEmitter::X86_64

#endif // EZPACKER_X86_64_CODE_EMITTER_H
