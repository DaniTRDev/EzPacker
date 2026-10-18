#ifndef EZPACKER_C_FRONTEND_FUNCTION_LOWERER_H
#define EZPACKER_C_FRONTEND_FUNCTION_LOWERER_H

#include "EzCompilerCommon.h"
#include "Operand/MirOperandBuilder.h"
#include <unordered_map>
#include <unordered_set>
#include <string>

// Forward declarations
struct Obj;
struct Node;
struct Type;
class MirBuilderContext;
class MirFunction;
class MirBlock;
class MirGlobalVar;
class MirRegister;
class MirOperand;
class MirType;
class MirTypeTable;
class StackFrameObject;

namespace EzCompiler
{
class DriverContext;

namespace CFrontend
{

/**
 * Lowers a single chibicc C AST function (struct Obj) into an EzPacker SSA MIR function.
 *
 * Responsibilities include:
 * - Registering formal parameters and local variables.
 * - Allocating static stack slots for address-taken variables.
 * - Lowering statements (blocks, if/else, loops, switch, return, goto).
 * - Lowering expressions (arithmetic, bitwise, comparisons, calls, casts, member access, ternary).
 * - Enforcing block termination invariants.
 */
class CFrontendFunctionLowerer
{
  public:
    CFrontendFunctionLowerer(DriverContext &driverCtx,
                             MirBuilderContext &bCtx,
                             MirFunction *mirFunc,
                             struct Obj *chibiFunc);

    /**
     * Executes the lowering pass on the function AST.
     */
    void lower();

  private:
    void collectAddressTaken(struct Node *node, std::unordered_set<struct Obj *> &addrTaken);
    MirBlock *createBlock(const char *prefix);
    MirGlobalVar *findGlobal(std::string_view name);
    MirGlobalVar *findOrCreateGlobal(std::string_view name, struct Type *ty);
    MirRegister *ensureRegister(MirOperand *op, MirType *fallbackType = nullptr, const std::string &hint = "reg");

    void lowerStatement(struct Node *node);
    MirOperand *lowerExpression(struct Node *node);

  private:
    DriverContext &m_driverCtx;
    MirBuilderContext &m_bCtx;
    MirFunction *m_func;
    struct Obj *m_chibiFunc;
    MirTypeTable *m_tt;
    MirOperandBuilder m_opBuilder;
    MirBlock *m_currentBlock{ nullptr };
    MirBlock *m_breakBlock{ nullptr };
    MirBlock *m_continueBlock{ nullptr };
    size_t m_blockCounter{ 0 };
    std::unordered_map<struct Obj *, MirRegister *> m_localVars;
    std::unordered_map<struct Obj *, StackFrameObject *> m_localStackSlots;
};

} // namespace CFrontend
} // namespace EzCompiler

#endif // EZPACKER_C_FRONTEND_FUNCTION_LOWERER_H

