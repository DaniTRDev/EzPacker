#ifndef EZPACKER_C_FRONTEND_FUNCTION_LOWERER_H
#define EZPACKER_C_FRONTEND_FUNCTION_LOWERER_H

#include "EzCompilerCommon.h"
#include "Debug/DebugInfo.h"
#include "Operand/MirOperandBuilder.h"
#include <unordered_map>
#include <unordered_set>
#include <string>

// Forward declarations
struct Obj;
struct Node;
struct Type;
struct Token;
class MirBuilderContext;
struct MirDebugMetadata;
class MirFunction;
class MirBlock;
class MirGlobalVar;
class MirRegister;
class MirOperand;
class MirType;
class MirTypeTable;
class StackFrameObject;
class SourceReference;

namespace EzCompiler
{
class DriverContext;

namespace CFrontend
{

/**
 * Shared source-file and debug-type registration state passed across the C frontend
 * during AST-to-MIR lowering.
 */
struct CFrontendDebugContext
{
    std::unordered_map<const void *, size_t> fileToSrcMgrId;      ///< chibicc File* -> SourceManager file ID.
    std::unordered_map<const void *, uint32_t> fileToDebugFileId; ///< chibicc File* -> DebugCompileUnit file ID.
    std::unordered_map<const Type *, uint64_t> visitedTypes;      ///< Cycle-safe C Type* -> DebugType::typeId.
    std::unordered_map<std::string, uint64_t> canonicalPrimitives; ///< Canonical primitive name -> DebugType::typeId.
    std::unordered_map<uint64_t, uint64_t> canonicalPointers;      ///< Pointee typeId -> pointer DebugType::typeId.
    uint64_t nextTypeId{ 1 };

    SourceReference *createSourceRefFromToken(DriverContext &ctx, const Token *tok) const;
    EzCore::Debug::DebugSourceLoc debugLocFromToken(DriverContext &ctx, const Token *tok) const;
    uint64_t registerCType(DriverContext &ctx, const Type *ty, MirDebugMetadata &meta);
};

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
                             struct Obj *chibiFunc,
                             CFrontendDebugContext *debugCtx = nullptr);

    /**
     * Executes the lowering pass on the function AST.
     * Returns true on success, false if any diagnostic error was recorded.
     */
    bool lower();

  private:
    void collectAddressTaken(struct Node *node, std::unordered_set<struct Obj *> &addrTaken);
    MirBlock *createBlock(const char *prefix);
    MirBlock *getOrCreateBlockForLabel(const std::string &uniqueLabel);
    MirGlobalVar *findGlobal(std::string_view name);
    MirGlobalVar *findOrCreateGlobal(std::string_view name, struct Type *ty);
    MirRegister *ensureRegister(MirOperand *op, MirType *fallbackType = nullptr, const std::string &hint = "reg");

    void lowerStatement(struct Node *node);
    MirOperand *lowerExpression(struct Node *node);
    MirRegister *lowerAddr(struct Node *node);

  private:
    DriverContext &m_driverCtx;
    MirBuilderContext &m_bCtx;
    MirFunction *m_func;
    struct Obj *m_chibiFunc;
    CFrontendDebugContext *m_debugCtx{ nullptr };
    MirTypeTable *m_tt;
    MirOperandBuilder m_opBuilder;
    MirBlock *m_currentBlock{ nullptr };
    MirBlock *m_breakBlock{ nullptr };
    MirBlock *m_continueBlock{ nullptr };
    size_t m_blockCounter{ 0 };
    std::unordered_map<struct Obj *, MirRegister *> m_localVars;
    std::unordered_map<struct Obj *, StackFrameObject *> m_localStackSlots;
    std::unordered_map<std::string, MirBlock *> m_labeledBlocks;
    bool m_hasError{ false };
};

} // namespace CFrontend
} // namespace EzCompiler

#endif // EZPACKER_C_FRONTEND_FUNCTION_LOWERER_H

