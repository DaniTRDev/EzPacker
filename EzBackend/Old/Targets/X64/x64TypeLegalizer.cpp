#include "x64TypeLegalizer.h"

/**
 * @brief Handler for x86_64 DIV, IDIV, and MUL instructions.
 *
 * Hardware constraint: These x64 instructions cannot take an immediate operand.
 * If the MIR contains `DIV v1, 10`, we must lower it to:
 *    MOV v2, 10
 *    DIV v1, v2
 */
static LegalizerHandlerResult handleX86DivAndMul(MirEmitter *emitter,
                                                 TypedPoolLinkedList<MirInstruction> *instrList,
                                                 TypedPoolLinkedList<MirInstruction>::Iterator it)
{
    MirInstruction *instr = *it;
    auto operands = instr->getOperands();

    // Safety check: ensure we have at least 2 operands (dest, src)
    if (operands->m_numElems < 2)
    {
        return { true, false, false }; // Error state
    }

    // Get the source operand (Operand 1)
    auto srcIt = ++operands->begin();
    MirOperand *srcOp = *srcIt;

    // Check if the source is an immediate integer
    if (srcOp->isOfType<MirInteger>())
    {
        // Create a new virtual register matching the immediate's type (e.g., i32 or i64)
        MirType *opType = srcOp->getMirType();
        MirRegister *tempReg = emitter->createVirtualRegister(opType);

        // Create the `MOV tempReg, imm` instruction
        MirInstruction *movInstr = emitter->emitMOV(tempReg, srcOp);
        instrList->appendBefore(it, movInstr);

        // Swap the immediate in the DIV/MUL instruction with the new virtual register
        srcIt.m_curr->m_object = tempReg;

        // Success: No errors, successfully legalized, instruction was modified
        return { false, true, true };
    }

    // If it's already a register, x86_64 supports it natively.
    // No changes were necessary.
    return { false, true, false };
}

/**
 * @brief Intercepts ALU instructions and hoists 64-bit immediates that
 * exceed the 32-bit signed limit into a virtual register.
 */
static LegalizerHandlerResult handleX64LargeImmediate(MirEmitter *emitter,
                                                      TypedPoolLinkedList<MirInstruction> *instrList,
                                                      TypedPoolLinkedList<MirInstruction>::Iterator it)
{
    MirInstruction *instr = *it;
    auto operands = instr->getOperands();
    bool modified = false;

    // We check all operands because CMP/TEST might have the immediate at index 1 or 0
    for (auto opIt = operands->begin(); opIt != operands->end(); ++opIt)
    {
        MirOperand *op = *opIt;

        if (op->isOfType<MirInteger>())
        {
            MirInteger *immOp = op->get<MirInteger>();

            // Only care about 64-bit types (32-bit types are inherently safe)
            if (immOp->getMirType()->getTotalSizeInBytes() == 8)
            {
                int64_t val = immOp->getValue();

                // Check if the value falls outside the 32-bit signed range
                if (val < INT32_MIN || val > INT32_MAX)
                {
                    // Hoist into a temporary register
                    MirRegister *tempReg = emitter->createVirtualRegister(immOp->getMirType());

                    // The MOV instruction CAN take a true 64-bit immediate in x86_64
                    MirInstruction *movInstr = emitter->emitMOV(tempReg, immOp);
                    instrList->appendBefore(it, movInstr);

                    // Replace the immediate in the original instruction
                    opIt.m_curr->m_object = tempReg;
                    modified = true;
                }
            }
        }
    }

    return { false, true, modified };
}

std::shared_ptr<LegalizerActionList> x64TypeLegalizer::getActionList(MirTypeTable *types)
{
    auto ret = std::make_shared<LegalizerActionList>();

    // Promote Booleans (i1) to Bytes (i8) across the board
    ret->setOperandActionForClass(MirInstructionCategory::Arithmetic, types->getInt1Type(), Action_PromoteOperand);
    ret->setOperandActionForClass(MirInstructionCategory::Bitwise, types->getInt1Type(), Action_PromoteOperand);
    ret->setOperandActionForClass(MirInstructionCategory::Compare, types->getInt1Type(), Action_PromoteOperand);
    ret->setOperandActionForClass(MirInstructionCategory::DataMovement, types->getInt1Type(), Action_PromoteOperand);
    ret->setOperandActionForClass(MirInstructionCategory::Memory, types->getInt1Type(), Action_PromoteOperand);

    // Map standard natively supported integer sizes
    std::vector<MirType *> nativeInts = { types->getInt8Type(),
                                          types->getInt16Type(),
                                          types->getInt32Type(),
                                          types->getInt64Type() };

    for (MirType *type : nativeInts)
    {
        ret->setOperandActionForClass(MirInstructionCategory::Arithmetic, type, Action_None);
        ret->setOperandActionForClass(MirInstructionCategory::Bitwise, type, Action_None);
        ret->setOperandActionForClass(MirInstructionCategory::Compare, type, Action_None);
        ret->setOperandActionForClass(MirInstructionCategory::DataMovement, type, Action_None);
        ret->setOperandActionForClass(MirInstructionCategory::Memory, type, Action_None);
        // Add ControlFlow if your JMP/CALL instructions take typed operands
    }

    // Standard ALU instructions don't support 64-bit type. The CPU takes a signed 32-bit immediate and sign extends it.
    ret->setOperandAction(MirInstructionOpCode::ADD, types->getInt64Type(), Action_TypeCustom);
    ret->setOperandAction(MirInstructionOpCode::SUB, types->getInt64Type(), Action_TypeCustom);
    ret->setOperandAction(MirInstructionOpCode::CMP, types->getInt64Type(), Action_TypeCustom);
    ret->setOperandAction(MirInstructionOpCode::TEST, types->getInt64Type(), Action_TypeCustom);
    ret->setOperandAction(MirInstructionOpCode::AND, types->getInt64Type(), Action_TypeCustom);
    ret->setOperandAction(MirInstructionOpCode::OR, types->getInt64Type(), Action_TypeCustom);
    ret->setOperandAction(MirInstructionOpCode::XOR, types->getInt64Type(), Action_TypeCustom);

    // 3. Custom Opcode Overrides
    // Division (DIV/IDIV) and Multiplication (MUL) lack immediate support.
    ret->setOperandAction(MirInstructionOpCode::DIV, types->getInt32Type(), Action_TypeCustom);
    ret->setOperandAction(MirInstructionOpCode::DIV, types->getInt64Type(), Action_TypeCustom);
    ret->setOperandAction(MirInstructionOpCode::IDIV, types->getInt32Type(), Action_TypeCustom);
    ret->setOperandAction(MirInstructionOpCode::IDIV, types->getInt64Type(), Action_TypeCustom);

    ret->setOperandAction(MirInstructionOpCode::MUL, types->getInt32Type(), Action_TypeCustom);
    ret->setOperandAction(MirInstructionOpCode::MUL, types->getInt64Type(), Action_TypeCustom);
    ret->setOperandAction(MirInstructionOpCode::IMUL, types->getInt64Type(), Action_TypeCustom);

    ret->setOperandAction(MirInstructionOpCode::STORE, types->getInt64Type(), Action_TypeCustom);
    
    return ret;
}

std::shared_ptr<LegalizerHandlerList> x64TypeLegalizer::getHandlerList(MirTypeTable *types)
{
    auto ret = std::make_shared<LegalizerHandlerList>();

    ret->addInstructionHandler(MirInstructionOpCode::DIV, handleX86DivAndMul);
    ret->addInstructionHandler(MirInstructionOpCode::IDIV, handleX86DivAndMul);
    ret->addInstructionHandler(MirInstructionOpCode::MUL, handleX86DivAndMul);
    ret->addInstructionHandler(MirInstructionOpCode::IMUL, handleX64LargeImmediate);

    ret->addInstructionHandler(MirInstructionOpCode::ADD, handleX64LargeImmediate);
    ret->addInstructionHandler(MirInstructionOpCode::SUB, handleX64LargeImmediate);
    ret->addInstructionHandler(MirInstructionOpCode::CMP, handleX64LargeImmediate);
    ret->addInstructionHandler(MirInstructionOpCode::TEST, handleX64LargeImmediate);
    ret->addInstructionHandler(MirInstructionOpCode::AND, handleX64LargeImmediate);
    ret->addInstructionHandler(MirInstructionOpCode::OR, handleX64LargeImmediate);
    ret->addInstructionHandler(MirInstructionOpCode::XOR, handleX64LargeImmediate);
    
    ret->addInstructionHandler(MirInstructionOpCode::STORE, handleX64LargeImmediate);

    return ret;
}
