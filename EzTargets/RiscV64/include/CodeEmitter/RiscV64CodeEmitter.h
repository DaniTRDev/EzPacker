#ifndef EZTARGETS_RISCV64_CODE_EMITTER_H
#define EZTARGETS_RISCV64_CODE_EMITTER_H

#include "EzTargetsRiscV64Common.h"
#include "GenericCodeEmitter.h"
#include "Encoding/RiscV64EncodingDesc.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "Operand/MirOperands.h"
#include <functional>
#include <vector>

namespace EzTargets::RiscV64
{

using EncodingResolver = std::function<const EncodingDesc *(const MirTargetInstructionDesc *)>;

/**
 * RISC-V 64 Machine Code Emitter implementing GenericCodeEmitter.
 * Encodes target-lowered machine instructions into binary sections by interpreting the
 * declarative EncodingDesc supplied for each instruction descriptor.
 */
class RiscV64CodeEmitter : public GenericCodeEmitter
{
  public:
    RiscV64CodeEmitter() = default;
    ~RiscV64CodeEmitter() override = default;

    void beginFunction(CodeEmitterContext *ctx, std::string_view name) override;
    void beginFunction(CodeEmitterContext *ctx, MirFunction *func) override;
    void bindLabel(MirId labelId) override;
    void endFunction(CodeEmitterContext *ctx) override;
    void endFunction(CodeEmitterContext *ctx, MirFunction *func) override;

    void emitInst(const MirTargetInstructionDesc *desc, std::span<MirOperand *const> operands) override;
    void setEncodingResolver(EncodingResolver resolver) { m_encodingResolver = std::move(resolver); }

  private:
    uint8_t mapRegister(MirRegister *reg, bool &isFpr, bool &isVr) const;
    bool tryEmitTableDriven(const MirTargetInstructionDesc *desc, std::span<MirOperand *const> operands);
    bool buildResolvedOperands(const EncodingDesc &enc,
                               std::span<MirOperand *const> operands,
                               std::vector<ResolvedOperand> &resolved) const;

  private:
    CodeEmitterContext *m_ctx{ nullptr };
    MirFunction *m_currentFunc{ nullptr };
    EncodingResolver m_encodingResolver;
};

} // namespace EzTargets::RiscV64

#endif // EZTARGETS_RISCV64_CODE_EMITTER_H
