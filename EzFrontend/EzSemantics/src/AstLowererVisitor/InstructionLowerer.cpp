#include "AstLowererVisitor/InstructionLowerer.h"
#include "AstLowererVisitor/AstLowererVisitor.h"

bool InstructionLowerer::lower(AstNode *node, LoweringContext *ctx)
{
    // We don't care if it's a call or a regular instruction, we can lower them both.
    Instruction *instruction = dynamic_cast<Instruction *>(node);
    const std::string_view &instrName = instruction->getInstructionName();
    std::string instrName2Lower = StrToLower(std::string(instrName.data(), instrName.size()));
    MirInstructionOpCode opcode = getOpCodeFromStr(instrName2Lower);

    if (opcode == MirInstructionOpCode::INVALID)
    {
        ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                             "Could not lower instruction because its mnemonic is invalid",
                                             "InstructionLowerer");
        return false;
    }

    MirInstruction *instr = ctx->getEmitter()->emit(opcode);
    for (const void *ptr : *instruction->getExpressions())
    {
        AstNode *casted = (AstNode *)ptr;
        if (!casted->accept(ctx->getOwnerLowererVisitor()))
        {
            ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                                 "Could not lower instruction because one operand failed to be lowered",
                                                 "InstructionLowerer");
            return false;
        }
        ctx->getEmitter()->pushOperandToInstruction(instr, ctx->popOperand());
    }

    return true;
}