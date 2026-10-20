#include "CFrontend/CFrontendFunctionLowerer.h"
#include "CFrontend/CFrontendTypeMapper.h"
#include "DriverContext.h"
#include "Block/MirBlock.h"
#include "Block/MirBlockBuilder.h"
#include "Function/MirFunction.h"
#include "Function/MirFunctionBuilder.h"
#include "GlobalVar/MirGlobalVar.h"
#include "GlobalVar/MirGlobalVarBuilder.h"
#include "Instruction/MirInstruction.h"
#include "Instruction/MirInstructionBuilder.h"
#include "Operand/MirOperand.h"
#include "Operand/MirOperands.h"
#include "Type/MirType.h"
#include "Type/MirTypeTable.h"
#include "FlexNumber/FlexInt.h"
#include "FlexNumber/FlexFloat.h"
#include "Function/MirFunctionStackFrame.h"
#include "chibicc.h"
#include <algorithm>
#include <vector>

namespace EzCompiler::CFrontend
{

static inline bool isUnsignedType(Type *ty)
{
    return ty != nullptr && (ty->is_unsigned || ty->kind == TY_BOOL);
}

CFrontendFunctionLowerer::CFrontendFunctionLowerer(DriverContext &driverCtx,
                                                   MirBuilderContext &bCtx,
                                                   MirFunction *mirFunc,
                                                   Obj *chibiFunc)
    : m_driverCtx(driverCtx),
      m_bCtx(bCtx),
      m_func(mirFunc),
      m_chibiFunc(chibiFunc),
      m_tt(driverCtx.getTypeTable()),
      m_opBuilder(&bCtx),
      m_currentBlock(mirFunc->getEntryPoint())
{
}

void CFrontendFunctionLowerer::collectAddressTaken(Node *node, std::unordered_set<Obj *> &addrTaken)
{
    if (!node)
        return;
    if (node->kind == ND_ADDR || node->kind == ND_MEMBER)
    {
        Node *cur = node->lhs;
        while (cur && cur->kind == ND_MEMBER)
        {
            cur = cur->lhs;
        }
        if (cur && cur->kind == ND_VAR && cur->var && cur->var->is_local)
        {
            addrTaken.insert(cur->var);
        }
    }
    collectAddressTaken(node->lhs, addrTaken);
    collectAddressTaken(node->rhs, addrTaken);
    collectAddressTaken(node->cond, addrTaken);
    collectAddressTaken(node->then, addrTaken);
    collectAddressTaken(node->els, addrTaken);
    collectAddressTaken(node->init, addrTaken);
    collectAddressTaken(node->inc, addrTaken);
    collectAddressTaken(node->body, addrTaken);
    collectAddressTaken(node->next, addrTaken);
    for (Node *n = node->args; n; n = n->next)
    {
        collectAddressTaken(n, addrTaken);
    }
}

void CFrontendFunctionLowerer::lower()
{
    std::unordered_set<Obj *> addrTaken;
    if (m_chibiFunc->body)
    {
        collectAddressTaken(m_chibiFunc->body, addrTaken);
    }

    // 1. Register formal parameters
    for (Obj *p = m_chibiFunc->params; p; p = p->next)
    {
        MirType *pType = mapType(m_tt, p->ty);
        MirRegister *pReg = m_opBuilder.buildVReg(pType, p->name ? p->name : "param");
        MirFunctionBuilder(&m_bCtx).addParam(m_func, pReg);
        if (addrTaken.contains(p) || p->ty->kind == TY_STRUCT || p->ty->kind == TY_UNION)
        {
            StackFrameObject *slot = m_func->getStackFrame()->createStaticStackObj(pType);
            m_localStackSlots[p] = slot;
            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .STORE(m_opBuilder.buildRef(slot), pReg);
        }
        else
        {
            m_localVars[p] = pReg;
        }
    }

    // 2. Register local variables
    for (Obj *v = m_chibiFunc->locals; v; v = v->next)
    {
        if (m_localVars.find(v) == m_localVars.end() && m_localStackSlots.find(v) == m_localStackSlots.end())
        {
            MirType *vType = mapType(m_tt, v->ty);
            if (addrTaken.contains(v) || v->ty->kind == TY_STRUCT || v->ty->kind == TY_UNION || v->ty->kind == TY_ARRAY)
            {
                StackFrameObject *slot = m_func->getStackFrame()->createStaticStackObj(vType);
                m_localStackSlots[v] = slot;
            }
            else
            {
                MirRegister *vReg = m_opBuilder.buildVReg(vType, v->name ? v->name : "local");
                m_localVars[v] = vReg;
            }
        }
    }

    // 3. Lower statements in function body
    if (m_chibiFunc->body)
    {
        lowerStatement(m_chibiFunc->body);
    }

    // 4. Ensure every basic block without terminator gets an appropriate return
    if (m_currentBlock && !isBlockTerminated(m_currentBlock))
    {
        MirInstructionBuilder inst(&m_bCtx, m_currentBlock, InsertionType::Append);
        if (m_func->getReturnType()->getKind() == MirTypeKind::Void)
        {
            inst.RET();
        }
        else
        {
            MirType *retType = m_func->getReturnType();
            uint16_t retBits = static_cast<uint16_t>(retType->getTotalSizeInBits());
            MirOperand *zero = m_opBuilder.buildInt(retType, FlexInt(0, retBits));
            inst.RET(zero);
        }
    }
}

MirBlock *CFrontendFunctionLowerer::createBlock(const char *prefix)
{
    MirBlockBuilder bb(&m_bCtx, m_func);
    return bb.build(nullptr, std::string(prefix) + "_" + std::to_string(++m_blockCounter));
}

MirBlock *CFrontendFunctionLowerer::getOrCreateBlockForLabel(const std::string &uniqueLabel)
{
    auto it = m_labeledBlocks.find(uniqueLabel);
    if (it != m_labeledBlocks.end())
    {
        return it->second;
    }
    MirBlock *b = createBlock(uniqueLabel.c_str());
    m_labeledBlocks[uniqueLabel] = b;
    return b;
}

MirGlobalVar *CFrontendFunctionLowerer::findGlobal(std::string_view name)
{
    for (MirGlobalVar *gv : m_bCtx.getGlobalVars())
    {
        if (gv && std::string_view(gv->getName()) == name)
            return gv;
    }
    return nullptr;
}

MirGlobalVar *CFrontendFunctionLowerer::findOrCreateGlobal(std::string_view name, struct Type *ty)
{
    MirGlobalVar *gv = findGlobal(name);
    if (!gv)
    {
        MirType *gType = mapType(m_tt, ty);
        MirGlobalVarBuilder gb(&m_bCtx);
        gb.setConstant(false);
        gb.setIsDeclaration(true);
        gv = gb.build(MirLinkage::External, gType, std::pmr::string(name, m_bCtx.getGlobalAllocator()));
        m_bCtx.appendGlobalVar(gv);
    }
    return gv;
}

MirRegister *CFrontendFunctionLowerer::ensureRegister(MirOperand *op, MirType *fallbackType, const std::string &hint)
{
    if (op && op->isOfType<MirRegister>())
    {
        return static_cast<MirRegister *>(op);
    }
    MirType *ty = (op && op->getMirType()) ? op->getMirType() : (fallbackType ? fallbackType : m_tt->i32());
    MirRegister *vreg = m_opBuilder.buildVReg(ty, hint);
    MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append).MOV(vreg, op);
    return vreg;
}

MirRegister *CFrontendFunctionLowerer::lowerAddr(Node *node)
{
    if (!node)
    {
        return m_opBuilder.buildVReg(m_tt->getPtr(m_tt->i8()), "null_addr");
    }

    switch (node->kind)
    {
        case ND_VLA_PTR:
        case ND_VAR:
        {
            if (node->var->is_local)
            {
                StackFrameObject *slot = nullptr;
                auto slotIt = m_localStackSlots.find(node->var);
                if (slotIt != m_localStackSlots.end())
                {
                    slot = slotIt->second;
                }
                else
                {
                    MirType *varType = mapType(m_tt, node->var->ty);
                    slot = m_func->getStackFrame()->createStaticStackObj(varType);
                    m_localStackSlots[node->var] = slot;
                    auto regIt = m_localVars.find(node->var);
                    if (regIt != m_localVars.end())
                    {
                        MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                                .STORE(m_opBuilder.buildRef(slot), regIt->second);
                    }
                }
                MirType *ptrType = m_tt->getPtr(mapType(m_tt, node->var->ty));
                MirRegister *addr = m_opBuilder.buildVReg(ptrType, "laddr");
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .MOV(addr, m_opBuilder.buildRef(slot));
                return addr;
            }
            else
            {
                MirGlobalVar *gv = findOrCreateGlobal(node->var->name, node->var->ty);
                MirType *ptrType = m_tt->getPtr(mapType(m_tt, node->var->ty));
                MirRegister *addr = m_opBuilder.buildVReg(ptrType, "gaddr");
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .MOV_ADDR(addr, m_opBuilder.buildRef(gv, 0));
                return addr;
            }
        }
        case ND_DEREF:
        {
            MirOperand *addrOp = lowerExpression(node->lhs);
            MirType *ptrType = (addrOp && addrOp->getMirType() && addrOp->getMirType()->getKind() == MirTypeKind::Pointer)
                                   ? addrOp->getMirType()
                                   : m_tt->getPtr(mapType(m_tt, node->ty));
            return ensureRegister(addrOp, ptrType, "deref_base");
        }
        case ND_MEMBER:
        {
            MirRegister *baseAddr = lowerAddr(node->lhs);
            if (node->member && node->member->offset != 0)
            {
                MirType *ptrType = baseAddr->getMirType();
                MirRegister *memberAddr = m_opBuilder.buildVReg(ptrType, "mem_addr");
                MirOperand *offsetImm = m_opBuilder.buildInt(m_tt->i64(), FlexInt(node->member->offset));
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .ADD(memberAddr, baseAddr, offsetImm);
                return memberAddr;
            }
            return baseAddr;
        }
        case ND_COMMA:
        {
            if (node->lhs)
                lowerExpression(node->lhs);
            return lowerAddr(node->rhs);
        }
        default:
        {
            MirOperand *val = lowerExpression(node);
            return ensureRegister(val, m_tt->getPtr(mapType(m_tt, node->ty)), "fallback_addr");
        }
    }
}

void CFrontendFunctionLowerer::lowerStatement(Node *node)
{
    if (!node)
        return;

    switch (node->kind)
    {
        case ND_BLOCK:
        {
            for (Node *s = node->body; s; s = s->next)
            {
                lowerStatement(s);
            }
            break;
        }
        case ND_RETURN:
        {
            MirInstructionBuilder inst(&m_bCtx, m_currentBlock, InsertionType::Append);
            if (node->lhs)
            {
                MirOperand *val = lowerExpression(node->lhs);
                inst.RET(val);
            }
            else
            {
                inst.RET();
            }
            break;
        }
        case ND_EXPR_STMT:
        {
            if (node->lhs)
            {
                lowerExpression(node->lhs);
            }
            break;
        }
        case ND_IF:
        {
            MirBlock *thenB = createBlock("if_then");
            MirBlock *elseB = node->els ? createBlock("if_else") : nullptr;
            MirBlock *joinB = createBlock("if_join");

            MirOperand *cond = lowerExpression(node->cond);
            MirRegister *condReg = ensureRegister(cond, m_tt->i1(), "if_cond");
            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .BR_COND(condReg,
                             m_opBuilder.buildRef(thenB),
                             m_opBuilder.buildRef(elseB ? elseB : joinB));

            m_currentBlock = thenB;
            lowerStatement(node->then);
            if (!isBlockTerminated(m_currentBlock))
            {
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .JMP(m_opBuilder.buildRef(joinB));
            }

            if (elseB)
            {
                m_currentBlock = elseB;
                lowerStatement(node->els);
                if (!isBlockTerminated(m_currentBlock))
                {
                    MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                            .JMP(m_opBuilder.buildRef(joinB));
                }
            }

            m_currentBlock = joinB;
            break;
        }
        case ND_FOR:
        {
            MirBlock *condB = createBlock("loop_cond");
            MirBlock *bodyB = createBlock("loop_body");
            MirBlock *stepB = node->inc ? createBlock("loop_step") : condB;
            MirBlock *exitB = createBlock("loop_exit");

            if (node->init)
            {
                lowerStatement(node->init);
            }

            if (!isBlockTerminated(m_currentBlock))
            {
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .JMP(m_opBuilder.buildRef(condB));
            }

            m_currentBlock = condB;
            if (node->cond)
            {
                MirOperand *cond = lowerExpression(node->cond);
                MirRegister *condReg = ensureRegister(cond, m_tt->i1(), "for_cond");
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .BR_COND(condReg,
                                 m_opBuilder.buildRef(bodyB),
                                 m_opBuilder.buildRef(exitB));
            }
            else
            {
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .JMP(m_opBuilder.buildRef(bodyB));
            }

            MirBlock *oldBreak = m_breakBlock;
            MirBlock *oldCont = m_continueBlock;
            m_breakBlock = exitB;
            m_continueBlock = stepB;

            m_currentBlock = bodyB;
            lowerStatement(node->then);
            if (!isBlockTerminated(m_currentBlock))
            {
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .JMP(m_opBuilder.buildRef(stepB));
            }

            if (node->inc)
            {
                m_currentBlock = stepB;
                lowerExpression(node->inc);
                if (!isBlockTerminated(m_currentBlock))
                {
                    MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                            .JMP(m_opBuilder.buildRef(condB));
                }
            }

            m_breakBlock = oldBreak;
            m_continueBlock = oldCont;
            m_currentBlock = exitB;
            break;
        }
        case ND_DO:
        {
            MirBlock *bodyB = createBlock("do_body");
            MirBlock *condB = createBlock("do_cond");
            MirBlock *exitB = createBlock("do_exit");

            if (!isBlockTerminated(m_currentBlock))
            {
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .JMP(m_opBuilder.buildRef(bodyB));
            }

            MirBlock *oldBreak = m_breakBlock;
            MirBlock *oldCont = m_continueBlock;
            m_breakBlock = exitB;
            m_continueBlock = condB;

            m_currentBlock = bodyB;
            lowerStatement(node->then);
            if (!isBlockTerminated(m_currentBlock))
            {
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .JMP(m_opBuilder.buildRef(condB));
            }

            m_currentBlock = condB;
            if (node->cond)
            {
                MirOperand *cond = lowerExpression(node->cond);
                MirRegister *condReg = ensureRegister(cond, m_tt->i1(), "do_cond");
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .BR_COND(condReg,
                                 m_opBuilder.buildRef(bodyB),
                                 m_opBuilder.buildRef(exitB));
            }
            else
            {
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .JMP(m_opBuilder.buildRef(bodyB));
            }

            m_breakBlock = oldBreak;
            m_continueBlock = oldCont;
            m_currentBlock = exitB;
            break;
        }
        case ND_SWITCH:
        {
            MirBlock *exitB = createBlock("switch_exit");
            if (node->brk_label)
            {
                m_labeledBlocks[node->brk_label] = exitB;
            }

            MirOperand *condVal = lowerExpression(node->cond);
            MirRegister *condReg = ensureRegister(condVal, m_tt->i32(), "switch_cond");

            // Collect all cases in order
            std::vector<Node *> cases;
            for (Node *c = node->case_next; c; c = c->case_next)
            {
                cases.push_back(c);
            }
            std::reverse(cases.begin(), cases.end());

            // Pre-create target blocks for each case
            for (Node *c : cases)
            {
                if (c->label)
                {
                    getOrCreateBlockForLabel(c->label);
                }
            }

            MirBlock *defaultB = nullptr;
            if (node->default_case && node->default_case->label)
            {
                defaultB = getOrCreateBlockForLabel(node->default_case->label);
            }

            // Emit dispatch checks
            for (size_t i = 0; i < cases.size(); ++i)
            {
                Node *c = cases[i];
                MirBlock *caseTarget = getOrCreateBlockForLabel(c->label);
                MirBlock *nextCheck = createBlock("switch_check");

                uint16_t bits = static_cast<uint16_t>(condReg->getMirType()->getTotalSizeInBits());
                if (c->begin == c->end)
                {
                    MirOperand *valImm = m_opBuilder.buildInt(condReg->getMirType(), FlexInt(static_cast<int64_t>(c->begin), static_cast<size_t>(bits)));
                    MirRegister *cmp = m_opBuilder.buildVReg(m_tt->i1(), "case_cmp");
                    MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                            .CMP_EQ(cmp, condReg, valImm);
                    MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                            .BR_COND(cmp, m_opBuilder.buildRef(caseTarget), m_opBuilder.buildRef(nextCheck));
                }
                else
                {
                    MirOperand *beginImm = m_opBuilder.buildInt(condReg->getMirType(), FlexInt(static_cast<int64_t>(c->begin), static_cast<size_t>(bits)));
                    MirOperand *endImm = m_opBuilder.buildInt(condReg->getMirType(), FlexInt(static_cast<int64_t>(c->end), static_cast<size_t>(bits)));
                    MirRegister *cmpLow = m_opBuilder.buildVReg(m_tt->i1(), "case_ge");
                    MirRegister *cmpHigh = m_opBuilder.buildVReg(m_tt->i1(), "case_le");
                    MirRegister *inRange = m_opBuilder.buildVReg(m_tt->i1(), "case_in_range");
                    MirInstructionBuilder inst(&m_bCtx, m_currentBlock, InsertionType::Append);
                    inst.CMP_SGE(cmpLow, condReg, beginImm);
                    inst.CMP_SLE(cmpHigh, condReg, endImm);
                    inst.AND(inRange, cmpLow, cmpHigh);
                    inst.BR_COND(inRange, m_opBuilder.buildRef(caseTarget), m_opBuilder.buildRef(nextCheck));
                }
                m_currentBlock = nextCheck;
            }

            // If no case matched: jump to default (if exists) or exit
            if (!isBlockTerminated(m_currentBlock))
            {
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .JMP(m_opBuilder.buildRef(defaultB ? defaultB : exitB));
            }

            MirBlock *oldBreak = m_breakBlock;
            m_breakBlock = exitB;

            // Lower switch body
            m_currentBlock = createBlock("switch_body_start");
            lowerStatement(node->then);

            if (!isBlockTerminated(m_currentBlock))
            {
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .JMP(m_opBuilder.buildRef(exitB));
            }

            m_breakBlock = oldBreak;
            m_currentBlock = exitB;
            break;
        }
        case ND_CASE:
        {
            if (node->label)
            {
                MirBlock *caseB = getOrCreateBlockForLabel(node->label);
                if (m_currentBlock && !isBlockTerminated(m_currentBlock))
                {
                    MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                            .JMP(m_opBuilder.buildRef(caseB));
                }
                m_currentBlock = caseB;
            }
            if (node->lhs)
            {
                lowerStatement(node->lhs);
            }
            break;
        }
        case ND_GOTO:
        {
            std::string lbl = node->unique_label ? node->unique_label : (node->label ? node->label : "");
            if (!lbl.empty())
            {
                MirBlock *target = getOrCreateBlockForLabel(lbl);
                if (m_currentBlock && !isBlockTerminated(m_currentBlock))
                {
                    MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                            .JMP(m_opBuilder.buildRef(target));
                }
                m_currentBlock = createBlock("after_goto");
            }
            break;
        }
        case ND_LABEL:
        {
            std::string lbl = node->unique_label ? node->unique_label : (node->label ? node->label : "label");
            MirBlock *lblBlock = getOrCreateBlockForLabel(lbl);
            if (m_currentBlock && !isBlockTerminated(m_currentBlock))
            {
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .JMP(m_opBuilder.buildRef(lblBlock));
            }
            m_currentBlock = lblBlock;
            if (node->lhs)
            {
                lowerStatement(node->lhs);
            }
            break;
        }
        case ND_GOTO_EXPR:
        {
            MirOperand *targetOp = lowerExpression(node->lhs);
            if (!m_labeledBlocks.empty())
            {
                MirRegister *targetReg = ensureRegister(targetOp, m_tt->getPtr(m_tt->i8()), "goto_target");
                std::vector<std::pair<std::string, MirBlock *>> targets(m_labeledBlocks.begin(), m_labeledBlocks.end());
                for (size_t i = 0; i < targets.size(); ++i)
                {
                    MirBlock *destB = targets[i].second;
                    MirBlock *nextCheck = (i + 1 < targets.size()) ? createBlock("computed_goto_next") : destB;
                    MirRegister *addrReg = m_opBuilder.buildVReg(m_tt->getPtr(m_tt->i8()), "lbl_addr_check");
                    MirInstructionBuilder inst(&m_bCtx, m_currentBlock, InsertionType::Append);
                    inst.MOV(addrReg, m_opBuilder.buildRef(destB));
                    MirRegister *cmp = m_opBuilder.buildVReg(m_tt->i1(), "is_dest");
                    inst.CMP_EQ(cmp, targetReg, addrReg);
                    inst.BR_COND(cmp, m_opBuilder.buildRef(destB), m_opBuilder.buildRef(nextCheck));
                    m_currentBlock = nextCheck;
                }
            }
            m_currentBlock = createBlock("after_goto_expr");
            break;
        }
        case ND_MEMZERO:
        {
            if (node->var)
            {
                StackFrameObject *slot = nullptr;
                auto slotIt = m_localStackSlots.find(node->var);
                if (slotIt != m_localStackSlots.end())
                {
                    slot = slotIt->second;
                }
                else
                {
                    MirType *varType = mapType(m_tt, node->var->ty);
                    slot = m_func->getStackFrame()->createStaticStackObj(varType);
                    m_localStackSlots[node->var] = slot;
                }
                MirType *ptrType = m_tt->getPtr(mapType(m_tt, node->var->ty));
                MirRegister *baseAddr = m_opBuilder.buildVReg(ptrType, "memzero_addr");
                MirInstructionBuilder inst(&m_bCtx, m_currentBlock, InsertionType::Append);
                inst.MOV(baseAddr, m_opBuilder.buildRef(slot));

                size_t sz = static_cast<size_t>(node->var->ty && node->var->ty->size > 0 ? node->var->ty->size : 0);
                if (sz > 0 && sz <= 64)
                {
                    size_t offset = 0;
                    while (sz - offset >= 8)
                    {
                        MirMemory *mem = m_opBuilder.buildMem(m_tt->i64(), baseAddr, FlexInt(static_cast<int64_t>(offset), 64));
                        MirOperand *zero = m_opBuilder.buildInt(m_tt->i64(), FlexInt(0, 64));
                        inst.STORE(mem, zero);
                        offset += 8;
                    }
                    if (sz - offset >= 4)
                    {
                        MirMemory *mem = m_opBuilder.buildMem(m_tt->i32(), baseAddr, FlexInt(static_cast<int64_t>(offset), 64));
                        MirOperand *zero = m_opBuilder.buildInt(m_tt->i32(), FlexInt(0, 32));
                        inst.STORE(mem, zero);
                        offset += 4;
                    }
                    if (sz - offset >= 2)
                    {
                        MirMemory *mem = m_opBuilder.buildMem(m_tt->i16(), baseAddr, FlexInt(static_cast<int64_t>(offset), 64));
                        MirOperand *zero = m_opBuilder.buildInt(m_tt->i16(), FlexInt(0, 16));
                        inst.STORE(mem, zero);
                        offset += 2;
                    }
                    if (sz - offset >= 1)
                    {
                        MirMemory *mem = m_opBuilder.buildMem(m_tt->i8(), baseAddr, FlexInt(static_cast<int64_t>(offset), 64));
                        MirOperand *zero = m_opBuilder.buildInt(m_tt->i8(), FlexInt(0, 8));
                        inst.STORE(mem, zero);
                        offset += 1;
                    }
                }
                else if (sz > 64)
                {
                    MirOperand *callee = m_opBuilder.buildRtSymbol(std::pmr::string("memset", m_bCtx.getGlobalAllocator()));
                    MirOperand *valOp = m_opBuilder.buildInt(m_tt->i32(), FlexInt(0, 32));
                    MirOperand *lenOp = m_opBuilder.buildInt(m_tt->i64(), FlexInt(static_cast<int64_t>(sz), 64));
                    inst.build(MirInstructionOpCode::CALL, nullptr, { baseAddr, callee, baseAddr, valOp, lenOp });
                }
            }
            break;
        }
        case ND_NULL_EXPR:
            break;
        default:
            lowerExpression(node);
            break;
    }
}

MirOperand *CFrontendFunctionLowerer::lowerExpression(Node *node)
{
    if (!node)
    {
        return m_opBuilder.buildInt(m_tt->i32(), FlexInt(0));
    }

    switch (node->kind)
    {
        case ND_NUM:
        {
            MirType *t = mapType(m_tt, node->ty);
            if (is_flonum(node->ty))
            {
                return m_opBuilder.buildFloat(t, FlexFloat(static_cast<double>(node->fval)));
            }
            uint16_t bits = static_cast<uint16_t>(t->getTotalSizeInBits());
            if (isUnsignedType(node->ty))
            {
                return m_opBuilder.buildInt(t, FlexInt(static_cast<uint64_t>(node->val), bits));
            }
            return m_opBuilder.buildInt(t, FlexInt(node->val, bits));
        }
        case ND_VLA_PTR:
        case ND_VAR:
        {
            if (node->var->is_local)
            {
                auto slotIt = m_localStackSlots.find(node->var);
                if (slotIt != m_localStackSlots.end())
                {
                    if (node->var->ty->kind == TY_ARRAY || node->var->ty->kind == TY_STRUCT || node->var->ty->kind == TY_UNION)
                    {
                        MirType *ptrType = m_tt->getPtr(mapType(m_tt, node->var->ty));
                        MirRegister *addr = m_opBuilder.buildVReg(ptrType, node->var->name ? node->var->name : "laddr");
                        MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                                .MOV(addr, m_opBuilder.buildRef(slotIt->second));
                        return addr;
                    }
                    MirType *t = mapType(m_tt, node->var->ty);
                    MirRegister *valReg = m_opBuilder.buildVReg(t, node->var->name ? node->var->name : "lval");
                    MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                            .LOAD(valReg, m_opBuilder.buildRef(slotIt->second));
                    return valReg;
                }
                auto it = m_localVars.find(node->var);
                if (it != m_localVars.end())
                {
                    return it->second;
                }
                MirType *t = mapType(m_tt, node->var->ty);
                MirRegister *reg = m_opBuilder.buildVReg(t, node->var->name ? node->var->name : "var");
                m_localVars[node->var] = reg;
                return reg;
            }
            else
            {
                MirGlobalVar *gv = findOrCreateGlobal(node->var->name, node->var->ty);
                MirRegister *valReg = m_opBuilder.buildVReg(mapType(m_tt, node->var->ty), "gval");
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .LOAD(valReg, m_opBuilder.buildRef(gv, 0));
                return valReg;
            }
        }
        case ND_ASSIGN:
        {
            MirOperand *rhs = lowerExpression(node->rhs);
            if ((node->lhs->kind == ND_VAR || node->lhs->kind == ND_VLA_PTR) && node->lhs->var->is_local)
            {
                auto slotIt = m_localStackSlots.find(node->lhs->var);
                if (slotIt != m_localStackSlots.end())
                {
                    MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                            .STORE(m_opBuilder.buildRef(slotIt->second), rhs);
                    return rhs;
                }
                auto it = m_localVars.find(node->lhs->var);
                MirRegister *dest = (it != m_localVars.end()) ? it->second : nullptr;
                if (!dest)
                {
                    dest = m_opBuilder.buildVReg(mapType(m_tt, node->lhs->var->ty), node->lhs->var->name);
                    m_localVars[node->lhs->var] = dest;
                }
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .MOV(dest, rhs);
                return dest;
            }
            else if ((node->lhs->kind == ND_VAR || node->lhs->kind == ND_VLA_PTR) && !node->lhs->var->is_local)
            {
                MirGlobalVar *gv = findOrCreateGlobal(node->lhs->var->name, node->lhs->var->ty);
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .STORE(m_opBuilder.buildRef(gv, 0), rhs);
                return rhs;
            }
            else if (node->lhs->kind == ND_DEREF)
            {
                MirOperand *addr = lowerExpression(node->lhs->lhs);
                MirType *ptrType = (addr && addr->getMirType() && addr->getMirType()->getKind() == MirTypeKind::Pointer)
                                       ? addr->getMirType()
                                       : m_tt->getPtr(rhs->getMirType());
                MirRegister *baseReg = addr->get<MirRegister>();
                if (!baseReg || !baseReg->getMirType() || baseReg->getMirType()->getKind() != MirTypeKind::Pointer)
                {
                    baseReg = m_opBuilder.buildVReg(ptrType, "base_tmp");
                    MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                            .MOV(baseReg, addr);
                }
                MirMemory *mem = m_opBuilder.buildMem(rhs->getMirType(), baseReg, FlexInt(0));
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .STORE(mem, rhs);
                return rhs;
            }
            else if (node->lhs->kind == ND_MEMBER)
            {
                MirRegister *addr = lowerAddr(node->lhs);
                MirMemory *mem = m_opBuilder.buildMem(rhs->getMirType(), addr, FlexInt(0));
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .STORE(mem, rhs);
                return rhs;
            }
            return rhs;
        }
        case ND_ADD:
        case ND_SUB:
        case ND_MUL:
        case ND_DIV:
        case ND_MOD:
        case ND_BITAND:
        case ND_BITOR:
        case ND_BITXOR:
        case ND_SHL:
        case ND_SHR:
        case ND_EQ:
        case ND_NE:
        case ND_LT:
        case ND_LE:
        {
            MirOperand *lhs = lowerExpression(node->lhs);
            MirOperand *rhs = lowerExpression(node->rhs);
            MirType *resType = mapType(m_tt, node->ty);
            MirRegister *dest = m_opBuilder.buildVReg(resType, "tmp");
            MirInstructionBuilder inst(&m_bCtx, m_currentBlock, InsertionType::Append);

            auto ensureReg = [&](MirOperand *op, const std::string &hint = "reg") -> MirRegister * {
                if (op && op->isOfType<MirRegister>())
                    return static_cast<MirRegister *>(op);
                MirType *ty = (op && op->getMirType()) ? op->getMirType() : resType;
                MirRegister *vreg = m_opBuilder.buildVReg(ty, hint);
                inst.MOV(vreg, op);
                return vreg;
            };

            lhs = ensureReg(lhs, "lhs");

            bool rhsIsLargeImm = false;
            if (rhs && rhs->isOfType<MirInteger>())
            {
                auto *imm = static_cast<MirInteger *>(rhs);
                int64_t val = imm->getValue().getI64();
                if (val < -2147483648LL || val > 2147483647LL)
                {
                    rhsIsLargeImm = true;
                }
            }

            bool rhsMustBeReg = rhsIsLargeImm ||
                                is_flonum(node->ty) ||
                                node->kind == ND_MUL ||
                                node->kind == ND_DIV ||
                                node->kind == ND_MOD ||
                                node->kind == ND_BITAND ||
                                node->kind == ND_BITOR ||
                                node->kind == ND_BITXOR ||
                                node->kind == ND_SHL ||
                                node->kind == ND_SHR;

            if (rhsMustBeReg && !rhs->isOfType<MirRegister>())
            {
                rhs = ensureReg(rhs, "rhs");
            }

            switch (node->kind)
            {
                case ND_ADD:
                    if (is_flonum(node->ty))
                        inst.FADD(dest, lhs, rhs);
                    else
                        inst.ADD(dest, lhs, rhs);
                    break;
                case ND_SUB:
                    if (is_flonum(node->ty))
                        inst.FSUB(dest, lhs, rhs);
                    else
                        inst.SUB(dest, lhs, rhs);
                    break;
                case ND_MUL:
                    if (is_flonum(node->ty))
                        inst.FMUL(dest, lhs, rhs);
                    else
                        inst.IMUL(dest, lhs, rhs);
                    break;
                case ND_DIV:
                    if (is_flonum(node->ty))
                        inst.FDIV(dest, lhs, rhs);
                    else if (isUnsignedType(node->ty))
                        inst.UDIV(dest, lhs, rhs);
                    else
                        inst.SDIV(dest, lhs, rhs);
                    break;
                case ND_MOD:
                    inst.REM(dest, lhs, rhs);
                    break;
                case ND_BITAND:
                    inst.AND(dest, lhs, rhs);
                    break;
                case ND_BITOR:
                    inst.OR(dest, lhs, rhs);
                    break;
                case ND_BITXOR:
                    inst.XOR(dest, lhs, rhs);
                    break;
                case ND_SHL:
                    inst.SHL(dest, lhs, rhs);
                    break;
                case ND_SHR:
                    if (isUnsignedType(node->ty))
                        inst.SHR(dest, lhs, rhs);
                    else
                        inst.SAR(dest, lhs, rhs);
                    break;
                case ND_EQ:
                    inst.CMP_EQ(dest, lhs, rhs);
                    break;
                case ND_NE:
                    inst.CMP_NE(dest, lhs, rhs);
                    break;
                case ND_LT:
                    if (isUnsignedType(node->lhs->ty))
                        inst.CMP_ULT(dest, lhs, rhs);
                    else
                        inst.CMP_SLT(dest, lhs, rhs);
                    break;
                case ND_LE:
                    if (isUnsignedType(node->lhs->ty))
                        inst.CMP_ULE(dest, lhs, rhs);
                    else
                        inst.CMP_SLE(dest, lhs, rhs);
                    break;
                default:
                    break;
            }
            return dest;
        }
        case ND_NEG:
        {
            MirOperand *val = lowerExpression(node->lhs);
            MirType *resType = mapType(m_tt, node->ty);
            MirRegister *dest = m_opBuilder.buildVReg(resType, "neg");
            uint16_t resBits = static_cast<uint16_t>(resType->getTotalSizeInBits());
            MirOperand *zero = m_opBuilder.buildInt(resType, FlexInt(0, resBits));
            MirInstructionBuilder inst(&m_bCtx, m_currentBlock, InsertionType::Append);
            MirRegister *zeroReg = m_opBuilder.buildVReg(resType, "zero");
            inst.MOV(zeroReg, zero);
            if (!val->isOfType<MirRegister>())
            {
                MirRegister *valReg = m_opBuilder.buildVReg(resType, "val");
                inst.MOV(valReg, val);
                val = valReg;
            }
            inst.SUB(dest, zeroReg, val);
            return dest;
        }
        case ND_NOT:
        {
            MirOperand *val = lowerExpression(node->lhs);
            MirType *valType = (val && val->getMirType()) ? val->getMirType() : m_tt->i32();
            MirType *resType = mapType(m_tt, node->ty);
            MirRegister *dest = m_opBuilder.buildVReg(resType, "not");
            uint16_t valBits = static_cast<uint16_t>(valType->getTotalSizeInBits());
            MirType *intValType = (valType->getKind() == MirTypeKind::Integer) ? valType : m_tt->getIntegerTypeBySize(valBits);
            MirOperand *zero = m_opBuilder.buildInt(intValType, FlexInt(0, valBits));
            MirInstructionBuilder inst(&m_bCtx, m_currentBlock, InsertionType::Append);
            if (!val->isOfType<MirRegister>())
            {
                MirRegister *valReg = m_opBuilder.buildVReg(valType, "val");
                inst.MOV(valReg, val);
                val = valReg;
            }
            inst.CMP_EQ(dest, val, zero);
            return dest;
        }
        case ND_BITNOT:
        {
            MirOperand *val = lowerExpression(node->lhs);
            MirType *resType = mapType(m_tt, node->ty);
            MirRegister *dest = m_opBuilder.buildVReg(resType, "bitnot");
            MirOperand *allOnes = m_opBuilder.buildInt(resType, FlexInt(-1, static_cast<uint16_t>(resType->getTotalSizeInBits())));
            MirInstructionBuilder inst(&m_bCtx, m_currentBlock, InsertionType::Append);
            if (!val->isOfType<MirRegister>())
            {
                MirRegister *valReg = m_opBuilder.buildVReg(resType, "val");
                inst.MOV(valReg, val);
                val = valReg;
            }
            MirRegister *onesReg = m_opBuilder.buildVReg(resType, "ones");
            inst.MOV(onesReg, allOnes);
            inst.XOR(dest, val, onesReg);
            return dest;
        }
        case ND_ADDR:
        {
            return lowerAddr(node->lhs);
        }
        case ND_MEMBER:
        {
            MirRegister *addr = lowerAddr(node);
            if (node->ty->kind == TY_ARRAY || node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION)
            {
                return addr;
            }
            MirType *resType = mapType(m_tt, node->ty);
            MirMemory *mem = m_opBuilder.buildMem(resType, addr, FlexInt(0));
            MirRegister *dest = m_opBuilder.buildVReg(resType, "member");
            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .LOAD(dest, mem);
            return dest;
        }
        case ND_DEREF:
        {
            if (node->ty->kind == TY_ARRAY || node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION ||
                node->ty->kind == TY_FUNC || node->ty->kind == TY_VLA)
            {
                return lowerExpression(node->lhs);
            }

            MirOperand *addr = lowerExpression(node->lhs);
            MirType *resType = mapType(m_tt, node->ty);
            MirType *ptrType = (addr && addr->getMirType() && addr->getMirType()->getKind() == MirTypeKind::Pointer)
                                   ? addr->getMirType()
                                   : m_tt->getPtr(resType);
            MirRegister *baseReg = addr->get<MirRegister>();
            if (!baseReg || !baseReg->getMirType() || baseReg->getMirType()->getKind() != MirTypeKind::Pointer)
            {
                baseReg = m_opBuilder.buildVReg(ptrType, "base_tmp");
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .MOV(baseReg, addr);
            }
            MirMemory *mem = m_opBuilder.buildMem(resType, baseReg, FlexInt(0));
            MirRegister *dest = m_opBuilder.buildVReg(resType, "deref");
            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .LOAD(dest, mem);
            return dest;
        }
        case ND_FUNCALL:
        {
            std::string_view calleeName = "func";
            if (node->lhs && node->lhs->kind == ND_VAR && node->lhs->var && node->lhs->var->name)
            {
                calleeName = node->lhs->var->name;
            }

            if (calleeName == "alloca" || calleeName == "__builtin_alloca")
            {
                MirOperand *szOp = node->args ? lowerExpression(node->args) : m_opBuilder.buildInt(m_tt->i64(), FlexInt(0, 64));
                MirRegister *szReg = ensureRegister(szOp, m_tt->i64(), "alloca_sz");
                MirType *ptrType = m_tt->getPtr(m_tt->i8());
                MirRegister *allocaDest = m_opBuilder.buildVReg(ptrType, "alloca_buf");
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .DALLOC(allocaDest, szReg);
                return allocaDest;
            }

            MirType *retType = mapType(m_tt, node->ty);
            bool isVoid = (retType && retType->getKind() == MirTypeKind::Void) ||
                          (node->ty && node->ty->kind == TY_VOID);
            MirRegister *dest = isVoid ? nullptr : m_opBuilder.buildVReg(retType, "call_ret");

            MirFunction *calleeFunc = nullptr;
            for (MirFunction *f : m_bCtx.getFunctions())
            {
                if (f && std::string_view(f->getName()) == calleeName)
                {
                    calleeFunc = f;
                    break;
                }
            }

            MirOperand *callee = nullptr;
            if (calleeFunc)
            {
                callee = m_opBuilder.buildRef(calleeFunc);
            }
            else
            {
                callee = m_opBuilder.buildRtSymbol(std::pmr::string(calleeName, m_bCtx.getGlobalAllocator()));
            }

            std::vector<MirOperand *> operands;
            if (dest)
            {
                operands.push_back(dest);
            }
            operands.push_back(callee);

            for (Node *arg = node->args; arg; arg = arg->next)
            {
                operands.push_back(lowerExpression(arg));
            }

            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .build(MirInstructionOpCode::CALL, nullptr, operands);
            return dest;
        }
        case ND_CAST:
        {
            MirOperand *src = lowerExpression(node->lhs);
            MirType *srcType = mapType(m_tt, node->lhs->ty);
            MirType *destType = mapType(m_tt, node->ty);

            if (!src || (destType && destType->getKind() == MirTypeKind::Void) || (node->ty && node->ty->kind == TY_VOID))
            {
                return nullptr;
            }

            if (srcType == destType)
            {
                return src;
            }

            if (src->isOfType<MirInteger>())
            {
                auto *imm = src->get<MirInteger>();
                uint16_t srcBits = static_cast<uint16_t>(srcType->getTotalSizeInBits());
                uint16_t destBits = static_cast<uint16_t>(destType->getTotalSizeInBits());
                bool srcUnsigned = isUnsignedType(node->lhs->ty);
                bool destUnsigned = isUnsignedType(node->ty);

                uint64_t uval = imm->getValue().getU64();
                if (srcBits < 64)
                {
                    uval &= ((1ULL << srcBits) - 1);
                }

                int64_t sval = 0;
                if (srcUnsigned)
                {
                    sval = static_cast<int64_t>(uval);
                }
                else
                {
                    if (srcBits < 64 && (uval & (1ULL << (srcBits - 1))))
                    {
                        sval = static_cast<int64_t>(uval | ~((1ULL << srcBits) - 1));
                    }
                    else
                    {
                        sval = static_cast<int64_t>(uval);
                    }
                }

                if (node->lhs->ty && node->lhs->ty->kind == TY_BOOL)
                {
                    sval = (sval != 0) ? 1 : 0;
                    uval = (uval != 0) ? 1 : 0;
                }
                if (node->ty && node->ty->kind == TY_BOOL)
                {
                    sval = (sval != 0) ? 1 : 0;
                    uval = (uval != 0) ? 1 : 0;
                }

                if (destType->getKind() == MirTypeKind::Integer)
                {
                    if (node->ty && node->ty->kind == TY_BOOL)
                    {
                        return m_opBuilder.buildInt(destType, FlexInt(static_cast<uint64_t>((uval != 0 || sval != 0) ? 1ULL : 0ULL), 1));
                    }
                    if (destUnsigned)
                    {
                        if (destBits < 64)
                        {
                            uval = static_cast<uint64_t>(sval) & ((1ULL << destBits) - 1);
                        }
                        else
                        {
                            uval = static_cast<uint64_t>(sval);
                        }
                        return m_opBuilder.buildInt(destType, FlexInt(uval, destBits));
                    }
                    else
                    {
                        if (destBits < 64 && (sval & (1LL << (destBits - 1))))
                        {
                            sval = sval | ~((1LL << destBits) - 1);
                        }
                        return m_opBuilder.buildInt(destType, FlexInt(sval, destBits));
                    }
                }
                else if (destType->getKind() == MirTypeKind::Pointer)
                {
                    uint16_t ptrBits = static_cast<uint16_t>(destType->getTotalSizeInBits());
                    MirInteger *ptrInt = m_opBuilder.buildInt(m_tt->getIntegerTypeBySize(ptrBits), FlexInt(static_cast<uint64_t>(sval), ptrBits));
                    MirRegister *dest = m_opBuilder.buildVReg(destType, "ptr_val");
                    MirInstructionBuilder inst(&m_bCtx, m_currentBlock, InsertionType::Append);
                    inst.MOV(dest, ptrInt);
                    return dest;
                }
            }

            MirRegister *dest = m_opBuilder.buildVReg(destType, "cast");
            MirInstructionBuilder inst(&m_bCtx, m_currentBlock, InsertionType::Append);

            if (!src->isOfType<MirRegister>())
            {
                MirRegister *srcReg = m_opBuilder.buildVReg(srcType, "cast_src");
                inst.MOV(srcReg, src);
                src = srcReg;
            }

            if (node->ty && node->ty->kind == TY_BOOL)
            {
                uint16_t srcBits = static_cast<uint16_t>(srcType->getTotalSizeInBits());
                MirType *intSrcType = (srcType->getKind() == MirTypeKind::Integer) ? srcType : m_tt->getIntegerTypeBySize(srcBits);
                MirOperand *zero = m_opBuilder.buildInt(intSrcType, FlexInt(0, srcBits));
                inst.CMP_NE(dest, src, zero);
                return dest;
            }

            if (destType->getTotalSizeInBits() < srcType->getTotalSizeInBits())
            {
                inst.TRUNC(dest, src);
            }
            else if (destType->getTotalSizeInBits() > srcType->getTotalSizeInBits())
            {
                if (isUnsignedType(node->lhs->ty))
                    inst.ZEXT(dest, src);
                else
                    inst.SEXT(dest, src);
            }
            else
            {
                inst.MOV(dest, src);
            }
            return dest;
        }
        case ND_COND:
        {
            MirOperand *cond = lowerExpression(node->cond);
            MirRegister *condReg = ensureRegister(cond, m_tt->i1(), "tern_cond");
            MirType *resType = mapType(m_tt, node->ty);
            MirRegister *dest = m_opBuilder.buildVReg(resType, "cond_val");

            MirBlock *thenB = createBlock("cond_then");
            MirBlock *elseB = createBlock("cond_else");
            MirBlock *joinB = createBlock("cond_join");

            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .BR_COND(condReg,
                             m_opBuilder.buildRef(thenB),
                             m_opBuilder.buildRef(elseB));

            m_currentBlock = thenB;
            MirOperand *thenVal = lowerExpression(node->then);
            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .MOV(dest, thenVal);
            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .JMP(m_opBuilder.buildRef(joinB));

            m_currentBlock = elseB;
            MirOperand *elseVal = lowerExpression(node->els);
            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .MOV(dest, elseVal);
            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .JMP(m_opBuilder.buildRef(joinB));

            m_currentBlock = joinB;
            return dest;
        }
        case ND_COMMA:
        {
            if (node->lhs)
                lowerExpression(node->lhs);
            if (node->rhs)
                return lowerExpression(node->rhs);
            return m_opBuilder.buildInt(m_tt->i32(), FlexInt(0));
        }
        case ND_STMT_EXPR:
        {
            MirOperand *lastVal = nullptr;
            for (Node *s = node->body; s; s = s->next)
            {
                if (s->kind == ND_EXPR_STMT)
                {
                    lastVal = lowerExpression(s->lhs);
                }
                else
                {
                    lowerStatement(s);
                }
            }
            return lastVal ? lastVal : m_opBuilder.buildInt(m_tt->i32(), FlexInt(0));
        }
        case ND_LOGAND:
        {
            MirType *resType = mapType(m_tt, node->ty);
            MirRegister *dest = m_opBuilder.buildVReg(resType, "logand");

            MirBlock *rhsB = createBlock("logand_rhs");
            MirBlock *falseB = createBlock("logand_false");
            MirBlock *joinB = createBlock("logand_join");

            MirOperand *lhsVal = lowerExpression(node->lhs);
            MirRegister *lhsReg = ensureRegister(lhsVal, m_tt->i1(), "logand_lhs");
            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .BR_COND(lhsReg,
                             m_opBuilder.buildRef(rhsB),
                             m_opBuilder.buildRef(falseB));

            m_currentBlock = rhsB;
            MirOperand *rhsVal = lowerExpression(node->rhs);
            MirType *rhsType = (rhsVal && rhsVal->getMirType()) ? rhsVal->getMirType() : resType;
            uint16_t rhsBits = static_cast<uint16_t>(rhsType->getTotalSizeInBits());
            MirOperand *zero = m_opBuilder.buildInt(rhsType, FlexInt(0, rhsBits));
            MirRegister *boolRhs = m_opBuilder.buildVReg(resType, "bool_rhs");
            MirRegister *rhsReg = ensureRegister(rhsVal, rhsType, "rhs_val");
            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .CMP_NE(boolRhs, rhsReg, zero);
            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .MOV(dest, boolRhs);
            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .JMP(m_opBuilder.buildRef(joinB));

            m_currentBlock = falseB;
            uint16_t resBits = static_cast<uint16_t>(resType->getTotalSizeInBits());
            MirOperand *zeroRes = m_opBuilder.buildInt(resType, FlexInt(0, resBits));
            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .MOV(dest, zeroRes);
            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .JMP(m_opBuilder.buildRef(joinB));

            m_currentBlock = joinB;
            return dest;
        }
        case ND_LOGOR:
        {
            MirType *resType = mapType(m_tt, node->ty);
            MirRegister *dest = m_opBuilder.buildVReg(resType, "logor");

            MirBlock *rhsB = createBlock("logor_rhs");
            MirBlock *trueB = createBlock("logor_true");
            MirBlock *joinB = createBlock("logor_join");

            MirOperand *lhsVal = lowerExpression(node->lhs);
            MirRegister *lhsReg = ensureRegister(lhsVal, m_tt->i1(), "logor_lhs");
            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .BR_COND(lhsReg,
                             m_opBuilder.buildRef(trueB),
                             m_opBuilder.buildRef(rhsB));

            m_currentBlock = trueB;
            uint16_t resBits = static_cast<uint16_t>(resType->getTotalSizeInBits());
            MirOperand *oneRes = m_opBuilder.buildInt(resType, FlexInt(1, resBits));
            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .MOV(dest, oneRes);
            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .JMP(m_opBuilder.buildRef(joinB));

            m_currentBlock = rhsB;
            MirOperand *rhsVal = lowerExpression(node->rhs);
            MirType *rhsType = (rhsVal && rhsVal->getMirType()) ? rhsVal->getMirType() : resType;
            uint16_t rhsBits = static_cast<uint16_t>(rhsType->getTotalSizeInBits());
            MirOperand *zero = m_opBuilder.buildInt(rhsType, FlexInt(0, rhsBits));
            MirRegister *boolRhs = m_opBuilder.buildVReg(resType, "bool_rhs");
            MirRegister *rhsReg = ensureRegister(rhsVal, rhsType, "rhs_val");
            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .CMP_NE(boolRhs, rhsReg, zero);
            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .MOV(dest, boolRhs);
            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .JMP(m_opBuilder.buildRef(joinB));

            m_currentBlock = joinB;
            return dest;
        }
        case ND_LABEL_VAL:
        {
            std::string lbl = node->unique_label ? node->unique_label : (node->label ? node->label : "");
            MirBlock *target = getOrCreateBlockForLabel(lbl);
            MirType *ptrType = m_tt->getPtr(m_tt->i8());
            MirRegister *addrReg = m_opBuilder.buildVReg(ptrType, "lbl_val");
            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                    .MOV(addrReg, m_opBuilder.buildRef(target));
            return addrReg;
        }
        case ND_MEMZERO:
            lowerStatement(node);
            return m_opBuilder.buildInt(m_tt->i32(), FlexInt(0, 32));
        case ND_NULL_EXPR:
            return m_opBuilder.buildInt(m_tt->i32(), FlexInt(0));
        default:
            return m_opBuilder.buildInt(m_tt->i32(), FlexInt(0));
    }
}

} // namespace EzCompiler::CFrontend

