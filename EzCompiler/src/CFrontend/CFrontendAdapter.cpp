#include "CFrontend/CFrontendAdapter.h"
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
#include "Operand/MirOperandBuilder.h"
#include "Operand/MirOperands.h"
#include "Type/MirType.h"
#include "Type/MirTypeTable.h"
#include "FlexNumber/FlexInt.h"
#include "FlexNumber/FlexFloat.h"

#include "chibicc.h"

#include "Function/MirFunctionStackFrame.h"

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>

namespace EzCompiler
{

namespace
{

MirType *mapType(MirTypeTable *tt, struct Type *ty)
{
    if (!ty)
        return tt->i32();

    switch (ty->kind)
    {
        case TY_VOID:
            return tt->_void();
        case TY_BOOL:
            return tt->i1();
        case TY_CHAR:
            return tt->i8();
        case TY_SHORT:
            return tt->i16();
        case TY_INT:
        case TY_ENUM:
            return tt->i32();
        case TY_LONG:
            return tt->i64();
        case TY_FLOAT:
            return tt->f32();
        case TY_DOUBLE:
        case TY_LDOUBLE:
            return tt->f64();
        case TY_PTR:
        case TY_ARRAY:
        case TY_VLA:
            if (ty->base)
                return tt->getPtr(mapType(tt, ty->base));
            return tt->getPtr(tt->_void());
        default:
            if (ty->size <= 1)
                return tt->i8();
            if (ty->size <= 2)
                return tt->i16();
            if (ty->size <= 4)
                return tt->i32();
            return tt->i64();
    }
}

bool isBlockTerminated(const MirBlock *block)
{
    if (!block || block->getInstrCount() == 0)
        return false;
    const MirInstruction *last = block->back();
    return last && bool(last->getFlags() & MirInstructionFlags::IsTerminator);
}

class FunctionLowerer
{
  public:
    FunctionLowerer(DriverContext &driverCtx,
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

    void collectAddressTaken(Node *node, std::unordered_set<Obj *> &addrTaken)
    {
        if (!node)
            return;
        if (node->kind == ND_ADDR && node->lhs && node->lhs->kind == ND_VAR && node->lhs->var && node->lhs->var->is_local)
        {
            addrTaken.insert(node->lhs->var);
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

    void lower()
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
            if (addrTaken.contains(p))
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
                if (addrTaken.contains(v))
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

  private:
    MirBlock *createBlock(const char *prefix)
    {
        MirBlockBuilder bb(&m_bCtx, m_func);
        return bb.build(nullptr, std::string(prefix) + "_" + std::to_string(++m_blockCounter));
    }

    MirGlobalVar *findGlobal(std::string_view name)
    {
        for (MirGlobalVar *gv : m_bCtx.getGlobalVars())
        {
            if (gv && std::string_view(gv->getName()) == name)
                return gv;
        }
        return nullptr;
    }

    MirGlobalVar *findOrCreateGlobal(std::string_view name, struct Type *ty)
    {
        MirGlobalVar *gv = findGlobal(name);
        if (!gv)
        {
            MirType *gType = mapType(m_tt, ty);
            MirGlobalVarBuilder gb(&m_bCtx);
            gb.setConstant(false);
            if (gType->getKind() == MirTypeKind::Integer)
            {
                gb.setInitializer(m_opBuilder.buildInt(gType, FlexInt(0, static_cast<uint16_t>(gType->getTotalSizeInBits()))));
            }
            gv = gb.build(MirLinkage::External, gType, std::pmr::string(name, m_bCtx.getGlobalAllocator()));
            m_bCtx.appendGlobalVar(gv);
        }
        return gv;
    }

    MirRegister *ensureRegister(MirOperand *op, MirType *fallbackType = nullptr, const std::string &hint = "reg")
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

    void lowerStatement(Node *node)
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
            default:
                // Other statements evaluated for side effects
                lowerExpression(node);
                break;
        }
    }

    MirOperand *lowerAddress(Node *node)
    {
        if (!node)
            return m_opBuilder.buildInt(m_tt->i64(), FlexInt(0));

        switch (node->kind)
        {
            case ND_VAR:
            {
                if (!node->var->is_local)
                {
                    MirGlobalVar *gv = findOrCreateGlobal(node->var->name, node->var->ty);
                    MirType *ptrType = m_tt->getPtr(mapType(m_tt, node->var->ty));
                    MirRegister *addr = m_opBuilder.buildVReg(ptrType, "gaddr");
                    MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                            .MOV_ADDR(addr, m_opBuilder.buildRef(gv, 0));
                    return addr;
                }
                else
                {
                    Obj *var = node->var;
                    StackFrameObject *slot = nullptr;
                    auto slotIt = m_localStackSlots.find(var);
                    if (slotIt != m_localStackSlots.end())
                    {
                        slot = slotIt->second;
                    }
                    else
                    {
                        MirType *varType = mapType(m_tt, var->ty);
                        slot = m_func->getStackFrame()->createStaticStackObj(varType);
                        m_localStackSlots[var] = slot;
                        auto regIt = m_localVars.find(var);
                        if (regIt != m_localVars.end())
                        {
                            MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                                    .STORE(m_opBuilder.buildRef(slot), regIt->second);
                        }
                    }
                    MirType *ptrType = m_tt->getPtr(mapType(m_tt, var->ty));
                    MirRegister *addr = m_opBuilder.buildVReg(ptrType, "laddr");
                    MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                            .MOV(addr, m_opBuilder.buildRef(slot));
                    return addr;
                }
            }
            case ND_DEREF:
            {
                return lowerExpression(node->lhs);
            }
            case ND_MEMBER:
            {
                MirOperand *baseAddr = lowerAddress(node->lhs);
                if (node->member && node->member->offset != 0)
                {
                    MirType *ptrType = m_tt->getPtr(mapType(m_tt, node->ty));
                    MirRegister *offAddr = m_opBuilder.buildVReg(ptrType, "mem_off");
                    MirRegister *baseReg = ensureRegister(baseAddr, m_tt->i64(), "base_off");
                    MirOperand *immOff = m_opBuilder.buildInt(m_tt->i64(), FlexInt(node->member->offset, 64));
                    MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                            .ADD(offAddr, baseReg, immOff);
                    return offAddr;
                }
                return baseAddr;
            }
            default:
                return lowerExpression(node);
        }
    }

    MirOperand *lowerExpression(Node *node)
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
                    if (t->getTotalSizeInBits() <= 32)
                    {
                        return m_opBuilder.buildFloat(t, FlexFloat(static_cast<float>(node->fval)));
                    }
                    return m_opBuilder.buildFloat(t, FlexFloat(static_cast<double>(node->fval)));
                }
                uint16_t bits = static_cast<uint16_t>(t->getTotalSizeInBits());
                if (node->ty && node->ty->is_unsigned)
                {
                    return m_opBuilder.buildInt(t, FlexInt(static_cast<uint64_t>(node->val), bits));
                }
                return m_opBuilder.buildInt(t, FlexInt(node->val, bits));
            }
            case ND_VAR:
            {
                if (node->var->is_local)
                {
                    auto slotIt = m_localStackSlots.find(node->var);
                    if (slotIt != m_localStackSlots.end())
                    {
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
                if (node->lhs->kind == ND_VAR && node->lhs->var->is_local)
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
                else if (node->lhs->kind == ND_VAR && !node->lhs->var->is_local)
                {
                    MirGlobalVar *gv = findOrCreateGlobal(node->lhs->var->name, node->lhs->var->ty);
                    MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                            .STORE(m_opBuilder.buildRef(gv, 0), rhs);
                    return rhs;
                }
                else if (node->lhs->kind == ND_DEREF || node->lhs->kind == ND_MEMBER)
                {
                    MirOperand *addr = (node->lhs->kind == ND_DEREF) ? lowerExpression(node->lhs->lhs) : lowerAddress(node->lhs);
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
                                    is_flonum(node->lhs->ty) ||
                                    is_flonum(node->rhs->ty) ||
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
                        else if (node->ty->is_unsigned)
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
                        if (node->ty->is_unsigned)
                            inst.SHR(dest, lhs, rhs);
                        else
                            inst.SAR(dest, lhs, rhs);
                        break;
                    case ND_EQ:
                        if (is_flonum(node->lhs->ty) || is_flonum(node->rhs->ty))
                            inst.FCMP(dest, lhs, rhs);
                        else
                            inst.CMP_EQ(dest, lhs, rhs);
                        break;
                    case ND_NE:
                        if (is_flonum(node->lhs->ty) || is_flonum(node->rhs->ty))
                        {
                            inst.FCMP(dest, lhs, rhs);
                            MirOperand *one = m_opBuilder.buildInt(resType, FlexInt(1, static_cast<uint16_t>(resType->getTotalSizeInBits())));
                            MirRegister *invDest = m_opBuilder.buildVReg(resType, "fcmp_ne");
                            inst.XOR(invDest, dest, one);
                            dest = invDest;
                        }
                        else
                        {
                            inst.CMP_NE(dest, lhs, rhs);
                        }
                        break;
                    case ND_LT:
                        if (is_flonum(node->lhs->ty) || is_flonum(node->rhs->ty))
                            inst.FCMP(dest, lhs, rhs);
                        else if (node->lhs->ty->is_unsigned)
                            inst.CMP_ULT(dest, lhs, rhs);
                        else
                            inst.CMP_SLT(dest, lhs, rhs);
                        break;
                    case ND_LE:
                        if (is_flonum(node->lhs->ty) || is_flonum(node->rhs->ty))
                            inst.FCMP(dest, lhs, rhs);
                        else if (node->lhs->ty->is_unsigned)
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
                MirOperand *zero = m_opBuilder.buildInt(valType, FlexInt(0, static_cast<uint16_t>(valType->getTotalSizeInBits())));
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
                return lowerAddress(node->lhs);
            }
            case ND_MEMBER:
            {
                if (node->ty->kind == TY_ARRAY || node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION ||
                    node->ty->kind == TY_FUNC || node->ty->kind == TY_VLA)
                {
                    return lowerAddress(node);
                }

                MirOperand *addr = lowerAddress(node);
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
                MirRegister *dest = m_opBuilder.buildVReg(resType, "mem_val");
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

                MirType *retType = mapType(m_tt, node->ty);
                MirRegister *dest = m_opBuilder.buildVReg(retType, "call_ret");

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
                operands.push_back(dest);
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

                if (srcType == destType)
                {
                    return src;
                }

                if (src->isOfType<MirInteger>() && destType->getKind() == MirTypeKind::Integer)
                {
                    auto *imm = src->get<MirInteger>();
                    uint16_t srcBits = static_cast<uint16_t>(srcType->getTotalSizeInBits());
                    uint16_t destBits = static_cast<uint16_t>(destType->getTotalSizeInBits());
                    bool srcUnsigned = (node->lhs->ty && node->lhs->ty->is_unsigned);
                    bool destUnsigned = (node->ty && node->ty->is_unsigned);

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
                    MirOperand *zero = m_opBuilder.buildInt(srcType, FlexInt(0, static_cast<uint16_t>(srcType->getTotalSizeInBits())));
                    inst.CMP_NE(dest, src, zero);
                    return dest;
                }

                if (destType->getTotalSizeInBits() < srcType->getTotalSizeInBits())
                {
                    inst.TRUNC(dest, src);
                }
                else if (destType->getTotalSizeInBits() > srcType->getTotalSizeInBits())
                {
                    if (node->lhs->ty->is_unsigned)
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
            default:
                return m_opBuilder.buildInt(m_tt->i32(), FlexInt(0));
        }
    }

  private:
    DriverContext &m_driverCtx;
    MirBuilderContext &m_bCtx;
    MirFunction *m_func;
    Obj *m_chibiFunc;
    MirTypeTable *m_tt;
    MirOperandBuilder m_opBuilder;
    MirBlock *m_currentBlock{ nullptr };
    MirBlock *m_breakBlock{ nullptr };
    MirBlock *m_continueBlock{ nullptr };
    size_t m_blockCounter{ 0 };
    std::unordered_map<Obj *, MirRegister *> m_localVars;
    std::unordered_map<Obj *, StackFrameObject *> m_localStackSlots;
};

} // namespace

CFrontendAdapter::CFrontendAdapter() = default;

void CFrontendAdapter::addIncludePath(std::string_view path)
{
    m_wrapper.addIncludePath(path);
}

void CFrontendAdapter::defineMacro(std::string_view name, std::string_view value)
{
    m_wrapper.defineMacro(name, value);
}

bool CFrontendAdapter::compileSourceToMir(DriverContext &ctx, std::string_view sourcePath, MirBuilderContext &outMirCtx)
{
    auto parseRes = m_wrapper.parseFile(sourcePath);
    if (!parseRes.success)
    {
        ctx.getDiagCollector()->error("CFrontend", "{}", parseRes.errorMessage);
        return false;
    }

    auto *typeTable = ctx.getTypeTable();
    MirFunctionBuilder funcBuilder(&outMirCtx);
    MirOperandBuilder opBuilder(&outMirCtx);

    // Pass 1: Global Variables and Function Declarations
    for (Obj *obj = parseRes.astRoot; obj; obj = obj->next)
    {
        if (!obj->is_function)
        {
            // Global Variable
            MirType *gType = mapType(typeTable, obj->ty);
            MirLinkage linkage = obj->is_static ? MirLinkage::Internal : MirLinkage::External;
            MirGlobalVarBuilder gb(&outMirCtx);
            gb.setConstant(false);

            if (obj->init_data)
            {
                int64_t initVal = 0;
                if (obj->ty->size >= 8)
                    initVal = *(const int64_t *)obj->init_data;
                else if (obj->ty->size >= 4)
                    initVal = *(const int32_t *)obj->init_data;
                else if (obj->ty->size >= 2)
                    initVal = *(const int16_t *)obj->init_data;
                else
                    initVal = *(const int8_t *)obj->init_data;

                MirType *initType = (gType->getKind() == MirTypeKind::Integer) ? gType : typeTable->i64();
                gb.setInitializer(opBuilder.buildInt(initType, FlexInt(initVal, static_cast<uint16_t>(initType->getTotalSizeInBits()))));
            }
            else if (obj->is_definition && gType->getKind() == MirTypeKind::Integer)
            {
                gb.setInitializer(opBuilder.buildInt(gType, FlexInt(0, static_cast<uint16_t>(gType->getTotalSizeInBits()))));
            }

            MirGlobalVar *gv = gb.build(linkage, gType, std::pmr::string(obj->name, outMirCtx.getGlobalAllocator()));
            outMirCtx.appendGlobalVar(gv);
        }
        else if (!obj->is_definition || !obj->body)
        {
            // External function prototype
            MirType *retType = mapType(typeTable, obj->ty->return_ty);
            std::vector<MirType *> paramTypes;
            for (Obj *p = obj->params; p; p = p->next)
            {
                paramTypes.push_back(mapType(typeTable, p->ty));
            }
            MirLinkage linkage = obj->is_static ? MirLinkage::Internal : MirLinkage::External;
            funcBuilder.declare(retType,
                                std::span<MirType *const>(paramTypes.data(), paramTypes.size()),
                                obj->name,
                                linkage);
        }
    }

    // Pass 2: Function Definitions
    for (Obj *obj = parseRes.astRoot; obj; obj = obj->next)
    {
        if (obj->is_function && obj->is_definition && obj->body)
        {
            MirType *retType = mapType(typeTable, obj->ty->return_ty);
            MirLinkage linkage = obj->is_static ? MirLinkage::Internal : MirLinkage::External;
            MirFunction *func = funcBuilder.build(retType, {}, obj->name, linkage);
            if (!func)
            {
                ctx.getDiagCollector()->error("CFrontend", "Failed to build MIR function for {}", obj->name);
                return false;
            }

            FunctionLowerer lowerer(ctx, outMirCtx, func, obj);
            lowerer.lower();
        }
    }

    return true;
}

bool CFrontendAdapter::compileStringToMir(DriverContext &ctx, std::string_view sourceCode, MirBuilderContext &outMirCtx, std::string_view virtualFileName)
{
    auto parseRes = m_wrapper.parseString(sourceCode, virtualFileName);
    if (!parseRes.success)
    {
        ctx.getDiagCollector()->error("CFrontend", "{}", parseRes.errorMessage);
        return false;
    }

    auto *typeTable = ctx.getTypeTable();
    MirFunctionBuilder funcBuilder(&outMirCtx);
    MirOperandBuilder opBuilder(&outMirCtx);

    // Pass 1: Globals & Function Prototypes
    for (Obj *obj = parseRes.astRoot; obj; obj = obj->next)
    {
        if (!obj->is_function)
        {
            MirType *gType = mapType(typeTable, obj->ty);
            MirLinkage linkage = obj->is_static ? MirLinkage::Internal : MirLinkage::External;
            MirGlobalVarBuilder gb(&outMirCtx);
            gb.setConstant(false);
            if (obj->init_data)
            {
                int64_t initVal = 0;
                if (obj->ty->size >= 8)
                    initVal = *(const int64_t *)obj->init_data;
                else if (obj->ty->size >= 4)
                    initVal = *(const int32_t *)obj->init_data;
                else if (obj->ty->size >= 2)
                    initVal = *(const int16_t *)obj->init_data;
                else
                    initVal = *(const int8_t *)obj->init_data;

                MirType *initType = (gType->getKind() == MirTypeKind::Integer) ? gType : typeTable->i64();
                gb.setInitializer(opBuilder.buildInt(initType, FlexInt(initVal, static_cast<uint16_t>(initType->getTotalSizeInBits()))));
            }
            else if (obj->is_definition && gType->getKind() == MirTypeKind::Integer)
            {
                gb.setInitializer(opBuilder.buildInt(gType, FlexInt(0, static_cast<uint16_t>(gType->getTotalSizeInBits()))));
            }
            MirGlobalVar *gv = gb.build(linkage, gType, std::pmr::string(obj->name, outMirCtx.getGlobalAllocator()));
            outMirCtx.appendGlobalVar(gv);
        }
        else if (!obj->is_definition || !obj->body)
        {
            MirType *retType = mapType(typeTable, obj->ty->return_ty);
            std::vector<MirType *> paramTypes;
            for (Obj *p = obj->params; p; p = p->next)
            {
                paramTypes.push_back(mapType(typeTable, p->ty));
            }
            MirLinkage linkage = obj->is_static ? MirLinkage::Internal : MirLinkage::External;
            funcBuilder.declare(retType,
                                std::span<MirType *const>(paramTypes.data(), paramTypes.size()),
                                obj->name,
                                linkage);
        }
    }

    // Pass 2: Function Definitions
    for (Obj *obj = parseRes.astRoot; obj; obj = obj->next)
    {
        if (obj->is_function && obj->is_definition && obj->body)
        {
            MirType *retType = mapType(typeTable, obj->ty->return_ty);
            MirLinkage linkage = obj->is_static ? MirLinkage::Internal : MirLinkage::External;
            MirFunction *func = funcBuilder.build(retType, {}, obj->name, linkage);
            if (!func)
            {
                ctx.getDiagCollector()->error("CFrontend", "Failed to build MIR function for {}", obj->name);
                return false;
            }

            FunctionLowerer lowerer(ctx, outMirCtx, func, obj);
            lowerer.lower();
        }
    }

    return true;
}

} // namespace EzCompiler
