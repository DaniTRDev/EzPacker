#ifndef EZTRIPLE_X86_64_TARGET_INSTRUCTION_SELECTOR_H
#define EZTRIPLE_X86_64_TARGET_INSTRUCTION_SELECTOR_H

#include "x86_64InstructionSelector.h"
#include <string_view>

class MirRegisterClass;

namespace EzTriple
{

    class X86_64TargetInstructionSelector : public x86_64InstructionSelector
    {
    public:
        explicit X86_64TargetInstructionSelector(TargetDesc *targetDesc);
        bool select(MirBuilderContext *ctx, MirInstruction *inst) override;

    private:
        MirRegisterClass *findClass(std::string_view name);
        bool selectJMP(MirBuilderContext *ctx, MirInstruction *inst);
        bool selectBR_COND(MirBuilderContext *ctx, MirInstruction *inst);
        bool selectCALL(MirBuilderContext *ctx, MirInstruction *inst);
        bool selectCMP(MirBuilderContext *ctx, MirInstruction *inst);
        bool selectLOAD(MirBuilderContext *ctx, MirInstruction *inst);
        bool selectSTORE(MirBuilderContext *ctx, MirInstruction *inst);
        bool selectPHI(MirBuilderContext *ctx, MirInstruction *inst);
        bool selectFloatALU(MirBuilderContext *ctx, MirInstruction *inst);
        bool selectFloatCvt(MirBuilderContext *ctx, MirInstruction *inst);
        bool selectMOV(MirBuilderContext *ctx, MirInstruction *inst);

        TargetDesc *m_targetDesc{ nullptr };
    };
} // namespace EzTriple

#endif // EZTRIPLE_X86_64_TARGET_INSTRUCTION_SELECTOR_H
