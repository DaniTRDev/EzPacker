#include "RegisterAllocator/RegisterAllocator.h"

bool RegisterAllocator::buildInterferenceGraph(LivenessResult *liveness,
                                               RegisterAllocatorCtx &ctx,
                                               std::pmr::list<MirBlock *> &blockList)
{
    for (MirBlock *block : blockList)
    {
        size_t blockId = block->getId();
        std::pmr::unordered_set<RegisterRef> live = liveness->m_liveOut[blockId];

        // Ensure all registers live at the exit of the block have nodes in G
        for (RegisterRef regRef : live)
        {
            addNode(regRef, ctx);
        }

        // Iterate backwards through instructions in the block
        const auto &instructions = block->getInstructions();
        for (auto it = instructions.rbegin(); it != instructions.rend(); ++it)
        {
            MirInstruction *inst = *it;

            // Extract defined and used registers for this instruction
            // (Replace these calls with your actual instruction API, e.g., inst->getDefs(), inst->getUses())
            std::pmr::vector<RegisterRef> defs = inst->getDefinedRegisters();
            const std::pmr::vector<RegisterRef> &uses = inst->getUsedRegisters();

            // Check if instruction is a register-to-register MOVE (u = v)
            const auto &operands = inst->getOperands();
            MirRegister *dst = operands[0]->get<MirRegister>(), *src = operands[1]->get<MirRegister>();

            bool isMove = (inst->getOpCode() == MOV) && dst && src;
            bool isCall = inst->getOpCode() == CALL;

            if (isCall)
            {
                const auto &callerSavedGPR = ctx.m_callignConv->getCallerSavedGPRegs();
                const auto &callerSavedFPR = ctx.m_callignConv->getCallerSavedFPRegs();

                for (auto &gpr : callerSavedGPR)
                    defs.push_back(RegisterRef::preg(gpr));

                for (auto &fpr : callerSavedFPR)
                    defs.push_back(RegisterRef::preg(fpr));
            }

            // Add interferences for defined registers
            for (const auto &defRegRef : defs)
            {
                addNode(defRegRef, ctx);

                for (const auto &liveRegRef : live)
                {
                    // Special case for MOVE inst (u <- v): u and v do NOT interfere simply because
                    // v is live at the def of u.
                    if (isMove && liveRegRef == src->getRef())
                    {
                        continue;
                    }

                    addEdge(defRegRef, liveRegRef, ctx);
                }
            }

            // Update live set for preceding instructions:
            // Remove DEFs (they are dead before this point unless used previously)
            for (const auto &defRegRef : defs)
            {
                live.erase(defRegRef);
            }

            // Add USEs (they become live before this instruction)
            for (const auto &useRegRef : uses)
            {
                addNode(useRegRef, ctx);
                live.insert(useRegRef);
            }
        }
    }

    return true;
}

void RegisterAllocator::addEdge(const RegisterRef &u, const RegisterRef &v, RegisterAllocatorCtx &ctx)
{
    addNode(u, ctx);
    addNode(v, ctx);

    ctx.m_iGraph[u].insert(v);
    ctx.m_iGraph[v].insert(u);
}

void RegisterAllocator::addNode(const RegisterRef &v, RegisterAllocatorCtx &ctx)
{
    ctx.m_iGraph.try_emplace(v, std::pmr::set<RegisterRef>(ctx.m_ctx->getGlobalAllocator()));
}