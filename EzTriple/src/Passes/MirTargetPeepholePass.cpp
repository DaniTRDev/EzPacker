#include "Passes/MirTargetPeepholePass.h"
#include "Block/MirBlock.h"
#include "Builder/MirBuilderContext.h"
#include "Descriptors/TargetDesc.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "FrameLowerer/MirFrameLowererPass.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Instruction/MirTargetInstructionDesc.h"
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Printer/MirPrinter.h"
#include <algorithm>
#include <format>

namespace
{

bool isMoveInstruction(MirInstruction *inst, MirRegisterRef &dst, MirRegisterRef &src)
{
    if (!inst)
    {
        return false;
    }
    bool isMov = (inst->getOpCode() == MirInstructionOpCode::MOV) ||
                 (inst->getTargetDesc() &&
                  (inst->getTargetDesc()->getTargetFlags() & MirInstructionFlags::IsMove));
    if (!isMov || inst->getOperandCount() < 2)
    {
        return false;
    }

    MirOperand *op0 = inst->getOperand(0);
    MirOperand *op1 = inst->getOperand(1);
    if (!op0 || !op1 || !op0->isOfType<MirRegister>() || !op1->isOfType<MirRegister>())
    {
        return false;
    }

    dst = op0->get<MirRegister>()->getRef();
    src = op1->get<MirRegister>()->getRef();
    return true;
}

bool isSameMemoryLocation(MirOperand *m1, MirOperand *m2)
{
    if (!m1 || !m2)
    {
        return false;
    }
    if (m1->getType() != m2->getType())
    {
        return false;
    }

    if (m1->isOfType<MirReference>())
    {
        auto *r1 = m1->get<MirReference>();
        auto *r2 = m2->get<MirReference>();
        return r1->getRefType() == r2->getRefType() &&
               r1->getRefId() == r2->getRefId() &&
               r1->getOffset() == r2->getOffset();
    }

    if (m1->isOfType<MirMemory>())
    {
        auto *mem1 = m1->get<MirMemory>();
        auto *mem2 = m2->get<MirMemory>();
        if (!mem1->getBase() || !mem2->getBase())
        {
            return false;
        }
        if (mem1->getBase()->getRef() != mem2->getBase()->getRef())
        {
            return false;
        }

        bool index1Has = (mem1->getIndex() != nullptr);
        bool index2Has = (mem2->getIndex() != nullptr);
        if (index1Has != index2Has)
        {
            return false;
        }
        if (index1Has)
        {
            if (mem1->getIndex()->getRef() != mem2->getIndex()->getRef())
            {
                return false;
            }
            if (mem1->getScale() != mem2->getScale())
            {
                return false;
            }
        }

        auto *d1 = mem1->getDisplacement();
        auto *d2 = mem2->getDisplacement();
        if (d1 && d2)
        {
            if (d1->getValue() != d2->getValue())
            {
                return false;
            }
        }
        else if (d1 != d2)
        {
            return false;
        }
        return true;
    }
    return false;
}

bool extractLoad(MirInstruction *inst, MirOperand *&memOp, MirRegisterRef &dstReg)
{
    if (!inst)
    {
        return false;
    }
    bool isLd = (inst->getOpCode() == MirInstructionOpCode::LOAD) ||
                (inst->getTargetDesc() &&
                 (inst->getTargetDesc()->getTargetFlags() & MirInstructionFlags::ReadsMemory));
    if (!isLd)
    {
        return false;
    }

    memOp = nullptr;
    bool foundReg = false;
    for (size_t i = 0; i < inst->getOperandCount(); ++i)
    {
        MirOperand *op = inst->getOperand(i);
        if (op && (op->isOfType<MirMemory>() || op->isOfType<MirReference>()))
        {
            memOp = op;
        }
        else if (op && op->isOfType<MirRegister>() && !foundReg)
        {
            dstReg = op->get<MirRegister>()->getRef();
            foundReg = true;
        }
    }
    return memOp != nullptr && foundReg;
}

bool extractStore(MirInstruction *inst, MirOperand *&memOp, MirRegisterRef &srcReg)
{
    if (!inst)
    {
        return false;
    }
    bool isSt = (inst->getOpCode() == MirInstructionOpCode::STORE) ||
                (inst->getTargetDesc() &&
                 (inst->getTargetDesc()->getTargetFlags() & MirInstructionFlags::WritesMemory));
    if (!isSt)
    {
        return false;
    }

    memOp = nullptr;
    bool foundReg = false;
    for (size_t i = 0; i < inst->getOperandCount(); ++i)
    {
        MirOperand *op = inst->getOperand(i);
        if (op && (op->isOfType<MirMemory>() || op->isOfType<MirReference>()))
        {
            memOp = op;
        }
        else if (op && op->isOfType<MirRegister>())
        {
            srcReg = op->get<MirRegister>()->getRef();
            foundReg = true;
        }
    }
    return memOp != nullptr && foundReg;
}

void collectMemoryRegisters(MirOperand *memOp, std::vector<MirRegisterRef> &regs)
{
    if (!memOp)
    {
        return;
    }
    if (memOp->isOfType<MirMemory>())
    {
        auto *mem = memOp->get<MirMemory>();
        if (mem->getBase())
        {
            regs.push_back(mem->getBase()->getRef());
        }
        if (mem->getIndex())
        {
            regs.push_back(mem->getIndex()->getRef());
        }
    }
}

} // namespace

MirTargetPeepholePass::MirTargetPeepholePass(MirBuilderContext *ctx, TargetDesc *targetDesc) :
    m_ctx(ctx), m_targetDesc(targetDesc)
{
}

const char *MirTargetPeepholePass::getName() const
{
    return "TargetPeepholePass";
}

MirPassIterationPlace MirTargetPeepholePass::getIterationPlace() const
{
    return MirPassIterationPlace::Function;
}

std::vector<std::type_index> MirTargetPeepholePass::getDependencies() const
{
    return { std::type_index(typeid(MirFrameLowererPass)) };
}

void MirTargetPeepholePass::reset()
{
    m_metrics = MirTargetPeepholeMetrics{};
}

MirPassResult MirTargetPeepholePass::run(IntrusiveLinkedList<MirFunction>::const_iterator it,
                                        MirPassManager *passManager)
{
    (void)passManager;
    MirFunction *func = *it;
    if (!func || !m_ctx)
    {
        return { .m_modifiedMir = false, .m_executed = true, .m_succeeded = false };
    }

    bool modified = false;
    constexpr size_t maxIterations = 16;
    size_t iteration = 0;
    bool blockChanged = true;

    while (blockChanged && iteration < maxIterations)
    {
        blockChanged = false;
        iteration++;

        for (MirBlock *block : func->getBlocks())
        {
            if (optimizeBlock(block))
            {
                blockChanged = true;
                modified = true;
            }
        }
    }

    if (modified && m_ctx->getDiagCollector()->isDiagEnabledForType(Diag_Trace))
    {
        auto log = m_ctx->getDiagCollector()->builder(Diag_Trace, getName());
        log << std::format("Target peephole pass completed on {}: {} self-moves, {} reciprocal moves, "
                           "{} redundant jumps, {} zero identities, {} spill reloads, {} redundant loads, "
                           "{} dead stores eliminated.",
                           func->getName(),
                           m_metrics.m_selfMovesEliminated,
                           m_metrics.m_reciprocalMovesEliminated,
                           m_metrics.m_redundantJumpsEliminated,
                           m_metrics.m_zeroIdentitiesEliminated,
                           m_metrics.m_spillReloadsForwarded,
                           m_metrics.m_redundantLoadsEliminated,
                           m_metrics.m_deadStoresEliminated);
    }

    return { .m_modifiedMir = modified, .m_executed = true, .m_succeeded = true };
}

bool MirTargetPeepholePass::optimizeBlock(MirBlock *block)
{
    if (!block)
    {
        return false;
    }

    bool changed = false;
    auto &instructions = block->getInstructions();

    for (auto it = instructions.begin(); it != instructions.end();)
    {
        MirInstruction *inst = *it;
        ++it;

        if (tryEliminateSelfMove(inst, block))
        {
            changed = true;
            continue;
        }

        if (tryEliminateReciprocalMove(inst, block))
        {
            changed = true;
            continue;
        }

        if (tryEliminateAdjacentJump(inst, block))
        {
            changed = true;
            continue;
        }

        if (tryEliminateZeroIdentity(inst, block))
        {
            changed = true;
            continue;
        }

        if (tryOptimizeMemoryAccesses(inst, block))
        {
            changed = true;
            continue;
        }
    }

    return changed;
}

bool MirTargetPeepholePass::tryEliminateSelfMove(MirInstruction *inst, MirBlock *block)
{
    (void)block;
    MirRegisterRef dst, src;
    if (isMoveInstruction(inst, dst, src) && dst == src)
    {
        MirInstructionBuilder(m_ctx, inst, InsertionType::InsertBefore).erase(inst);
        m_metrics.m_selfMovesEliminated++;
        return true;
    }
    return false;
}

bool MirTargetPeepholePass::tryEliminateReciprocalMove(MirInstruction *inst, MirBlock *block)
{
    (void)block;
    MirRegisterRef dst1, src1;
    if (!isMoveInstruction(inst, dst1, src1))
    {
        return false;
    }

    for (MirInstruction *cur = inst->getNext(); cur != nullptr; cur = cur->getNext())
    {
        MirRegisterRef dst2, src2;
        if (isMoveInstruction(cur, dst2, src2))
        {
            if (dst2 == src1 && src2 == dst1)
            {
                MirInstructionBuilder(m_ctx, cur, InsertionType::InsertBefore).erase(cur);
                m_metrics.m_reciprocalMovesEliminated++;
                return true;
            }
        }

        std::pmr::vector<MirRegisterRef> defs(m_ctx->getGlobalAllocator());
        cur->getDefinedRegisters(defs);
        if (std::find(defs.begin(), defs.end(), dst1) != defs.end() ||
            std::find(defs.begin(), defs.end(), src1) != defs.end())
        {
            break;
        }

        bool isBarrier = (cur->getOpCode() == MirInstructionOpCode::CALL) ||
                         (cur->getTargetDesc() &&
                          (cur->getTargetDesc()->getTargetFlags() &
                           (MirInstructionFlags::IsCall | MirInstructionFlags::IsTerminator)));
        if (isBarrier)
        {
            break;
        }
    }

    return false;
}

bool MirTargetPeepholePass::tryEliminateAdjacentJump(MirInstruction *inst, MirBlock *block)
{
    if (!block || !block->getNext())
    {
        return false;
    }

    bool isBranch = (inst->getOpCode() == MirInstructionOpCode::JMP) ||
                    (inst->getTargetDesc() &&
                     (inst->getTargetDesc()->getTargetFlags() & MirInstructionFlags::IsBranch));
    bool isTerminator = (inst->getOpCode() == MirInstructionOpCode::JMP) ||
                        (inst->getTargetDesc() &&
                         (inst->getTargetDesc()->getTargetFlags() & MirInstructionFlags::IsTerminator));

    if (!isBranch && !isTerminator)
    {
        return false;
    }

    if (inst->getTargetDesc() &&
        (inst->getTargetDesc()->getTargetFlags() & MirInstructionFlags::ReadsCPUFlags))
    {
        return false;
    }

    for (size_t i = 0; i < inst->getOperandCount(); ++i)
    {
        MirOperand *op = inst->getOperand(i);
        if (op && op->isOfType<MirReference>())
        {
            auto *ref = op->get<MirReference>();
            if (ref->isBlock() && ref->getRefId() == block->getNext()->getId())
            {
                MirInstructionBuilder(m_ctx, inst, InsertionType::InsertBefore).erase(inst);
                m_metrics.m_redundantJumpsEliminated++;
                return true;
            }
        }
    }

    return false;
}

bool MirTargetPeepholePass::tryEliminateZeroIdentity(MirInstruction *inst, MirBlock *block)
{
    (void)block;
    if (inst->getOperandCount() < 3)
    {
        return false;
    }

    MirOperand *op0 = inst->getOperand(0);
    MirOperand *op1 = inst->getOperand(1);
    MirOperand *op2 = inst->getOperand(2);

    if (!op0 || !op1 || !op2 || !op0->isOfType<MirRegister>() || !op1->isOfType<MirRegister>() ||
        !op2->isOfType<MirInteger>())
    {
        return false;
    }

    MirRegisterRef dst = op0->get<MirRegister>()->getRef();
    MirRegisterRef src = op1->get<MirRegister>()->getRef();
    if (dst != src)
    {
        return false;
    }

    if (!op2->get<MirInteger>()->getValue().isZero())
    {
        return false;
    }

    std::string name;
    if (inst->getTargetDesc())
    {
        name = inst->getTargetDesc()->getName();
    }
    else
    {
        name = inst->getOpCodeName();
    }
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    bool isIdentityOp = (name.find("add") != std::string::npos) ||
                        (name.find("sub") != std::string::npos) ||
                        (name.find("or") != std::string::npos) ||
                        (name.find("xor") != std::string::npos) ||
                        (name.find("shl") != std::string::npos) ||
                        (name.find("shr") != std::string::npos);

    if (!isIdentityOp)
    {
        return false;
    }

    if (inst->getTargetDesc() &&
        (inst->getTargetDesc()->getTargetFlags() & MirInstructionFlags::WritesCPUFlags))
    {
        return false;
    }

    MirInstructionBuilder(m_ctx, inst, InsertionType::InsertBefore).erase(inst);
    m_metrics.m_zeroIdentitiesEliminated++;
    return true;
}

bool MirTargetPeepholePass::tryOptimizeMemoryAccesses(MirInstruction *inst, MirBlock *block)
{
    (void)block;
    MirOperand *mem1 = nullptr;
    MirRegisterRef reg1;

    bool isSt = extractStore(inst, mem1, reg1);
    bool isLd = !isSt && extractLoad(inst, mem1, reg1);

    if (!isSt && !isLd)
    {
        return false;
    }

    std::vector<MirRegisterRef> addressRegs;
    collectMemoryRegisters(mem1, addressRegs);

    if (isSt)
    {
        // Case A: Store followed by load to same address with same register (forwarding)
        // Case B: Store followed by store to same address (dead store)
        for (MirInstruction *cur = inst->getNext(); cur != nullptr; cur = cur->getNext())
        {
            MirOperand *mem2 = nullptr;
            MirRegisterRef reg2;

            if (extractLoad(cur, mem2, reg2))
            {
                if (isSameMemoryLocation(mem1, mem2) && reg1 == reg2)
                {
                    // Reload of value already in reg2
                    MirInstructionBuilder(m_ctx, cur, InsertionType::InsertBefore).erase(cur);
                    m_metrics.m_spillReloadsForwarded++;
                    return true;
                }
            }
            else if (extractStore(cur, mem2, reg2))
            {
                if (isSameMemoryLocation(mem1, mem2))
                {
                    // Overwritten without intervening read: inst is dead
                    MirInstructionBuilder(m_ctx, inst, InsertionType::InsertBefore).erase(inst);
                    m_metrics.m_deadStoresEliminated++;
                    return true;
                }
            }

            // Invalidation check
            std::pmr::vector<MirRegisterRef> defs(m_ctx->getGlobalAllocator());
            cur->getDefinedRegisters(defs);

            // If reg1 is clobbered, store value is no longer available
            if (std::find(defs.begin(), defs.end(), reg1) != defs.end())
            {
                break;
            }

            // If base or index is clobbered, address is no longer identical
            bool addrClobbered = false;
            for (const auto &areg : addressRegs)
            {
                if (std::find(defs.begin(), defs.end(), areg) != defs.end())
                {
                    addrClobbered = true;
                    break;
                }
            }
            if (addrClobbered)
            {
                break;
            }

            // Memory clobber
            bool writesMem = (cur->getOpCode() == MirInstructionOpCode::STORE) ||
                             (cur->getTargetDesc() &&
                              (cur->getTargetDesc()->getTargetFlags() & MirInstructionFlags::WritesMemory));
            bool isCall = (cur->getOpCode() == MirInstructionOpCode::CALL) ||
                          (cur->getTargetDesc() &&
                           (cur->getTargetDesc()->getTargetFlags() & MirInstructionFlags::IsCall));
            if (writesMem || isCall)
            {
                break;
            }
        }
    }
    else if (isLd)
    {
        // Case C: Consecutive reloads from same address
        for (MirInstruction *cur = inst->getNext(); cur != nullptr; cur = cur->getNext())
        {
            MirOperand *mem2 = nullptr;
            MirRegisterRef reg2;

            if (extractLoad(cur, mem2, reg2))
            {
                if (isSameMemoryLocation(mem1, mem2) && reg1 == reg2)
                {
                    MirInstructionBuilder(m_ctx, cur, InsertionType::InsertBefore).erase(cur);
                    m_metrics.m_redundantLoadsEliminated++;
                    return true;
                }
            }

            std::pmr::vector<MirRegisterRef> defs(m_ctx->getGlobalAllocator());
            cur->getDefinedRegisters(defs);

            if (std::find(defs.begin(), defs.end(), reg1) != defs.end())
            {
                break;
            }

            bool addrClobbered = false;
            for (const auto &areg : addressRegs)
            {
                if (std::find(defs.begin(), defs.end(), areg) != defs.end())
                {
                    addrClobbered = true;
                    break;
                }
            }
            if (addrClobbered)
            {
                break;
            }

            bool writesMem = (cur->getOpCode() == MirInstructionOpCode::STORE) ||
                             (cur->getTargetDesc() &&
                              (cur->getTargetDesc()->getTargetFlags() & MirInstructionFlags::WritesMemory));
            bool isCall = (cur->getOpCode() == MirInstructionOpCode::CALL) ||
                          (cur->getTargetDesc() &&
                           (cur->getTargetDesc()->getTargetFlags() & MirInstructionFlags::IsCall));
            if (writesMem || isCall)
            {
                break;
            }
        }
    }

    return false;
}

void MirTargetPeepholePass::printResult()
{
    if (!m_ctx->getDiagCollector()->isDiagEnabledForType(Diag_Debug))
    {
        return;
    }

    auto log = m_ctx->getDiagCollector()->builder(Diag_Debug, getName());
    log << std::format("Total machine instructions eliminated: {}", m_metrics.totalEliminated());
}
