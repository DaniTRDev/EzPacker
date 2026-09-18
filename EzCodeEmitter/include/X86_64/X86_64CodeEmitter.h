#ifndef EZPACKER_X86_64_CODE_EMITTER_H
#define EZPACKER_X86_64_CODE_EMITTER_H

#include "EzCodeEmitterCommon.h"
#include "GenericCodeEmitter.h"
#include "X86_64/X86_64Encoding.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "Operand/MirOperands.h"
#include <functional>

namespace EzCodeEmitter::X86_64
{

/**
 * Physical register mapping function translating MirRegisterRef / physical ID into X86_64::Reg.
 */
using RegMapper = std::function<Reg(size_t physId)>;

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
    void bindLabel(MirId labelId) override;
    void endFunction(CodeEmitterContext *ctx) override;
    void emitInst(MirTargetInstructionDesc *desc, std::span<MirOperand *> operands) override;

    /**
     * Sets a custom physical register ID mapper.
     */
    void setRegMapper(RegMapper mapper) { m_regMapper = std::move(mapper); }

  private:
    Reg mapRegister(MirRegister *reg) const;
    MemoryOperand mapMemory(MirMemory *mem) const;
    MemoryOperand mapOperandToMemory(MirOperand *op) const;

  private:
    CodeEmitterContext *m_ctx{ nullptr };
    RegMapper m_regMapper;
};

} // namespace EzCodeEmitter::X86_64

#endif // EZPACKER_X86_64_CODE_EMITTER_H
