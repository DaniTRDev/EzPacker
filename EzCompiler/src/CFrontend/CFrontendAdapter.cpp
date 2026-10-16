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

#include <unordered_map>
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
            return tt->i64(); // 64-bit address space
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

    void lower()
    {
        // 1. Register formal parameters
        for (Obj *p = m_chibiFunc->params; p; p = p->next)
        {
            MirType *pType = mapType(m_tt, p->ty);
            MirRegister *pReg = m_opBuilder.buildVReg(pType, p->name ? p->name : "param");
            MirFunctionBuilder(&m_bCtx).addParam(m_func, pReg);
            m_localVars[p] = pReg;
        }

        // 2. Register local variables
        for (Obj *v = m_chibiFunc->locals; v; v = v->next)
        {
            if (m_localVars.find(v) == m_localVars.end())
            {
                MirType *vType = mapType(m_tt, v->ty);
                MirRegister *vReg = m_opBuilder.buildVReg(vType, v->name ? v->name : "local");
                m_localVars[v] = vReg;
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
                MirOperand *zero = m_opBuilder.buildInt(m_func->getReturnType(), FlexInt(0));
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
            gb.setInitializer(m_opBuilder.buildInt(gType, FlexInt(0, static_cast<uint16_t>(gType->getTotalSizeInBits()))));
            gv = gb.build(MirLinkage::External, gType, std::pmr::string(name, m_bCtx.getGlobalAllocator()));
            m_bCtx.appendGlobalVar(gv);
        }
        return gv;
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
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .BR_COND(cond,
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
                    MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                            .BR_COND(cond,
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
                    MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                            .BR_COND(cond,
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
                    return m_opBuilder.buildFloat(t, FlexFloat(static_cast<double>(node->fval)));
                }
                return m_opBuilder.buildInt(t, FlexInt(node->val, static_cast<uint16_t>(t->getTotalSizeInBits())));
            }
            case ND_VAR:
            {
                if (node->var->is_local)
                {
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
                else if (node->lhs->kind == ND_DEREF)
                {
                    MirOperand *addr = lowerExpression(node->lhs->lhs);
                    MirRegister *baseReg = addr->get<MirRegister>();
                    if (!baseReg)
                    {
                        baseReg = m_opBuilder.buildVReg(m_tt->i64(), "base_tmp");
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

                switch (node->kind)
                {
                    case ND_ADD:
                        inst.ADD(dest, lhs, rhs);
                        break;
                    case ND_SUB:
                        inst.SUB(dest, lhs, rhs);
                        break;
                    case ND_MUL:
                        inst.MUL(dest, lhs, rhs);
                        break;
                    case ND_DIV:
                        if (node->ty->is_unsigned)
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
                        inst.CMP_EQ(dest, lhs, rhs);
                        break;
                    case ND_NE:
                        inst.CMP_NE(dest, lhs, rhs);
                        break;
                    case ND_LT:
                        if (node->lhs->ty->is_unsigned)
                            inst.CMP_ULT(dest, lhs, rhs);
                        else
                            inst.CMP_SLT(dest, lhs, rhs);
                        break;
                    case ND_LE:
                        if (node->lhs->ty->is_unsigned)
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
                MirOperand *zero = m_opBuilder.buildInt(resType, FlexInt(0));
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .SUB(dest, zero, val);
                return dest;
            }
            case ND_NOT:
            {
                MirOperand *val = lowerExpression(node->lhs);
                MirType *resType = mapType(m_tt, node->ty);
                MirRegister *dest = m_opBuilder.buildVReg(resType, "not");
                MirOperand *zero = m_opBuilder.buildInt(resType, FlexInt(0));
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .CMP_EQ(dest, val, zero);
                return dest;
            }
            case ND_BITNOT:
            {
                MirOperand *val = lowerExpression(node->lhs);
                MirType *resType = mapType(m_tt, node->ty);
                MirRegister *dest = m_opBuilder.buildVReg(resType, "bitnot");
                MirOperand *allOnes = m_opBuilder.buildInt(resType, FlexInt(-1, static_cast<uint16_t>(resType->getTotalSizeInBits())));
                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .XOR(dest, val, allOnes);
                return dest;
            }
            case ND_ADDR:
            {
                if (node->lhs->kind == ND_VAR && !node->lhs->var->is_local)
                {
                    MirGlobalVar *gv = findOrCreateGlobal(node->lhs->var->name, node->lhs->var->ty);
                    MirRegister *addr = m_opBuilder.buildVReg(m_tt->i64(), "gaddr");
                    MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                            .MOV_ADDR(addr, m_opBuilder.buildRef(gv, 0));
                    return addr;
                }
                // Fallback for local address-of
                return m_opBuilder.buildInt(m_tt->i64(), FlexInt(0));
            }
            case ND_DEREF:
            {
                MirOperand *addr = lowerExpression(node->lhs);
                MirType *resType = mapType(m_tt, node->ty);
                MirRegister *baseReg = addr->get<MirRegister>();
                if (!baseReg)
                {
                    baseReg = m_opBuilder.buildVReg(m_tt->i64(), "base_tmp");
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

                MirRegister *dest = m_opBuilder.buildVReg(destType, "cast");
                MirInstructionBuilder inst(&m_bCtx, m_currentBlock, InsertionType::Append);

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
                MirType *resType = mapType(m_tt, node->ty);
                MirRegister *dest = m_opBuilder.buildVReg(resType, "cond_val");

                MirBlock *thenB = createBlock("cond_then");
                MirBlock *elseB = createBlock("cond_else");
                MirBlock *joinB = createBlock("cond_join");

                MirInstructionBuilder(&m_bCtx, m_currentBlock, InsertionType::Append)
                        .BR_COND(cond,
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

                gb.setInitializer(opBuilder.buildInt(gType, FlexInt(initVal, static_cast<uint16_t>(gType->getTotalSizeInBits()))));
            }
            else
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
                gb.setInitializer(opBuilder.buildInt(gType, FlexInt(initVal, static_cast<uint16_t>(gType->getTotalSizeInBits()))));
            }
            else
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
