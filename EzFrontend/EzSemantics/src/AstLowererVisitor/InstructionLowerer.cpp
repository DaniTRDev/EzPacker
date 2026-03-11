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
                                             "InstructionLowerer",
                                             node->getSourceRef());
        return false;
    }

    MirInstruction *loweredInstr = ctx->getEmitter()->emit(opcode);
    TypedPoolSlice<MirOperand> *operandList = loweredInstr->getOperands();

    for (AstNode *ptr : *instruction->getExpressions())
    {
        if (!ptr->accept(ctx->getOwnerLowererVisitor()))
        {
            ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                                 "Could not lower instruction because one operand failed to be lowered",
                                                 "InstructionLowerer",
                                                 node->getSourceRef());
            return false;
        }
        ctx->getEmitter()->pushOperandToInstruction(loweredInstr, ctx->popOperand());
    }

    size_t operandCount = operandList->m_numElems;
    MirOperand *first = operandCount > 0 ? *operandList->begin() : nullptr;
    MirOperand *second = operandCount > 1 ? *(operandList->begin() + 1) : nullptr;

    if (!ctx->getEmitter()->areInstructionOperandsLegal(getMeta(loweredInstr->getOpCode()),
                                                        first,
                                                        second,
                                                        operandCount))
    {
        ctx->getSemanticContext()->emitError(ErrorSeverity::Fatal,
                                             "Instruction is illegal",
                                             "MirEmitter::emit",
                                             node->getSourceRef());
        return false;
    }

    return true;
}