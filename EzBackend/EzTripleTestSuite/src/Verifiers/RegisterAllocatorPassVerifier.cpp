#include "Verifiers/RegisterAllocatorPassVerifier.h"

RegisterAllocatorPassVerifier::RegisterAllocatorPassVerifier(MirBuilderContext *ctx, MirRegisterAllocatorPass *pass) :
    MirPassVerifier(pass), m_ctx(ctx)
{
}

RegisterAllocatorPassVerifier &
RegisterAllocatorPassVerifier::verifyNoVirtualRegistersRemain(const std::pmr::list<MirBlock *> &blockList)
{
    for (MirBlock *block : blockList)
    {
        for (const MirInstruction *instr : block->getInstructions())
        {
            for (size_t i = 0; i < instr->getOperands().size(); ++i)
            {
                const MirOperand *op = instr->getOperands()[i];
                if (op->isOfType<MirRegister>())
                {
                    const auto *reg = op->get<MirRegister>();
                    EXPECT_FALSE(reg->isVirtual())
                            << "Instruction '" << instr->toString() << "' in block #" << block->getId()
                            << " still contains virtual register %v" << reg->getRegId() << " at operand index " << i;
                }
            }
        }
    }
    return *this;
}

RegisterAllocatorPassVerifier &
RegisterAllocatorPassVerifier::verifyAllocationMappingComplete(RegisterAllocatorCtx *allocCtx)
{
    for (const auto &[node, neighbors] : allocCtx->m_iGraph)
    {
        if (node.isVirtual())
        {
            bool isAllocated = allocCtx->m_allocatedRegs.contains(node);
            bool isSpilled = allocCtx->m_spilledRegs.contains(node);

            EXPECT_TRUE(isAllocated || isSpilled) << "Virtual register %v" << node.getId()
                                                  << " was neither assigned a physical register nor marked as spilled!";

            if (isAllocated)
            {
                RegisterRef physReg = allocCtx->m_allocatedRegs.at(node);
                EXPECT_TRUE(physReg.isPhysical())
                        << "Virtual register %v" << node.getId() << " was assigned a non-physical register ref!";
            }
        }
    }
    return *this;
}

RegisterAllocatorPassVerifier &
RegisterAllocatorPassVerifier::verifyNoInterferenceConflicts(RegisterAllocatorCtx *allocCtx)
{
    for (const auto &[node, neighbors] : allocCtx->m_iGraph)
    {
        if (!allocCtx->m_allocatedRegs.contains(node))
            continue; // Skip spilled virtual nodes

        RegisterRef colorU = allocCtx->m_allocatedRegs.at(node);

        for (const RegisterRef &neighbor : neighbors)
        {
            if (!allocCtx->m_allocatedRegs.contains(neighbor))
                continue;

            RegisterRef colorV = allocCtx->m_allocatedRegs.at(neighbor);

            EXPECT_NE(colorU, colorV) << "Interference conflict! Register " << (node.isVirtual() ? "%v" : "%p")
                                      << node.getId() << " and Register " << (neighbor.isVirtual() ? "%v" : "%p")
                                      << neighbor.getId()
                                      << " interfere in the graph but were both assigned physical register %p"
                                      << colorU.getId();
        }
    }
    return *this;
}

RegisterAllocatorPassVerifier &
RegisterAllocatorPassVerifier::verifySpillingCorrectness(RegisterAllocatorCtx *allocCtx,
                                                         const std::pmr::list<MirBlock *> &blockList)
{
    // 1. Ensure all spilled registers have an allocated stack object slot
    for (const auto &[spilledReg, stackSlot] : allocCtx->m_spilledRegs)
    {
        EXPECT_NE(stackSlot, nullptr) << "Spilled virtual register %v" << spilledReg.getId()
                                      << " has a nullptr StackFrameObject slot!";
    }

    // 2. Ensure instructions referencing spilled slots have generated LOAD or STORE operations
    for (const auto &[spilledReg, stackSlot] : allocCtx->m_spilledRegs)
    {
        size_t memoryOpsFound = 0;

        for (MirBlock *block : blockList)
        {
            for (const MirInstruction *instr : block->getInstructions())
            {
                MirInstructionOpCode op = instr->getOpCode();
                if (op == MirInstructionOpCode::STORE || op == MirInstructionOpCode::LOAD)
                {
                    for (const MirOperand *opnd : instr->getOperands())
                    {
                        if (opnd->isOfType<MirReference>())
                        {
                            memoryOpsFound++;
                        }
                    }
                }
            }
        }

        EXPECT_GT(memoryOpsFound, 0u) << "Spilled register %v" << spilledReg.getId()
                                      << " has no corresponding LOAD or STORE spill code inserted in the block list!";
    }

    return *this;
}

RegisterAllocatorPassVerifier &
RegisterAllocatorPassVerifier::verifyReservedRegistersNotAssigned(RegisterAllocatorCtx *allocCtx)
{
    for (const auto &[node, assignedPhysReg] : allocCtx->m_allocatedRegs)
    {
        // Only check virtual register nodes that received an allocation
        if (node.isVirtual())
        {
            EXPECT_FALSE(allocCtx->m_reservedRegs.contains(assignedPhysReg))
                    << "Virtual register %v" << node.getId() << " was illegally assigned reserved physical register %p"
                    << assignedPhysReg.getId() << "!";
        }
    }
    return *this;
}

RegisterAllocatorPassVerifier &
RegisterAllocatorPassVerifier::verifyFramePointerReservedOnDAlloc(RegisterAllocatorCtx *allocCtx)
{
    MirFunction *func = allocCtx->m_targetFunction;
    CallingConvDesc *callingConv = func->getCallingConv();
    if (callingConv && callingConv->hasFramePointer(func))
    {
        RegisterRef fpReg = callingConv->getFramePointerReg();

        EXPECT_TRUE(allocCtx->m_reservedRegs.contains(fpReg))
                << "Function requires a Frame Pointer (m_needsFramePointer is true), but the FP register %p"
                << fpReg.getId() << " was not present in m_reservedRegs!";
    }
    return *this;
}