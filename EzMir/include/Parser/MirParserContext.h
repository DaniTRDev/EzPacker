#ifndef EZMIR_MIR_PARSER_CONTEXT_H
#define EZMIR_MIR_PARSER_CONTEXT_H

#include "EzMirCommon.h"
#include "Parser/MirAstNodes.h"
#include <memory_resource>
#include <string_view>
#include <unordered_map>
#include <vector>

class MirBuilderContext;
class DiagnosticCollector;
class GenericSourceManager;
class SourceReference;
class MirFunction;
class MirBlock;
class MirInstruction;
class MirRegister;
class MirGlobalVar;
class MirType;
class MirRegisterClass;

namespace EzMir
{

enum class SymbolKind
{
    Register,
    BasicBlock,
    Function,
    GlobalVar
};

struct UnresolvedReference
{
    std::pmr::string m_symbolName;
    SourceReference *m_ref{ nullptr };
    MirInstruction *m_targetInstruction{ nullptr };
    size_t m_operandIndex{ 0 };
    SymbolKind m_kind{ SymbolKind::BasicBlock };

    UnresolvedReference(std::string_view name,
                        MirInstruction *inst,
                        size_t opIdx,
                        SymbolKind kind,
                        SourceReference *ref,
                        std::pmr::memory_resource *mr) :
        m_symbolName(name, mr),
        m_ref(ref),
        m_targetInstruction(inst),
        m_operandIndex(opIdx),
        m_kind(kind)
    {
    }
};

/**
 * Symbol resolution table, scope tracker, and forward reference patcher for MIR parsing.
 */
class MirParserContext
{
  public:
    MirParserContext(MirBuilderContext *bCtx,
                     DiagnosticCollector *diagCollector,
                     std::pmr::memory_resource *arena,
                     GenericSourceManager *sourceMgr = nullptr,
                     size_t sourceId = 0);

    // Scoping
    void enterFunction(MirFunction *func);
    void exitFunction();

    // Source reference creation helper
    SourceReference *createRef(size_t offset, size_t length);

    // Type resolution
    MirType *resolveType(const Ast::MirAstType *astType);

    // Register mapping
    MirRegister *declareRegister(std::string_view name,
                                 MirType *type,
                                 SourceReference *ref,
                                 MirRegisterClass *regClass = nullptr);
    MirRegister *resolveRegister(std::string_view name, SourceReference *ref);
    MirRegister *getOrCreateRegister(std::string_view name,
                                     MirType *type,
                                     SourceReference *ref,
                                     MirRegisterClass *regClass = nullptr);

    // Basic block mapping & forward references
    MirBlock *declareBlock(std::string_view name, SourceReference *ref);
    MirBlock *getOrCreateBlock(std::string_view name, SourceReference *ref);
    MirBlock *resolveBlock(std::string_view name, SourceReference *ref);

    // Globals & Functions
    MirGlobalVar *declareGlobal(std::string_view name, MirGlobalVar *var);
    MirGlobalVar *resolveGlobal(std::string_view name, SourceReference *ref);

    MirFunction *declareFunction(std::string_view name, MirFunction *func);
    MirFunction *resolveFunction(std::string_view name, SourceReference *ref);

    // Forward reference recording & resolution
    void recordForwardReference(std::string_view name,
                                MirInstruction *inst,
                                size_t operandIdx,
                                SymbolKind kind,
                                SourceReference *ref);
    bool resolvePendingFunctionFixups();
    bool resolveAllPendingFixups();

    void recordError() { ++m_errorCount; }
    bool hasErrors() const { return m_errorCount > 0; }
    size_t getErrorCount() const { return m_errorCount; }

    MirBuilderContext *getBuilderContext() const { return m_bCtx; }
    DiagnosticCollector *getDiagCollector() const { return m_diag; }
    std::pmr::memory_resource *getArena() const { return m_arena; }
    MirFunction *getCurrentFunction() const { return m_currentFunction; }
    GenericSourceManager *getSourceManager() const { return m_sourceMgr; }
    size_t getSourceId() const { return m_sourceId; }

  private:
    MirBuilderContext *m_bCtx{ nullptr };
    DiagnosticCollector *m_diag{ nullptr };
    std::pmr::memory_resource *m_arena{ nullptr };
    GenericSourceManager *m_sourceMgr{ nullptr };
    size_t m_sourceId{ 0 };
    MirFunction *m_currentFunction{ nullptr };

    // Function-scoped symbol tables
    std::pmr::unordered_map<std::pmr::string, MirRegister *> m_registers;
    std::pmr::unordered_map<std::pmr::string, MirBlock *> m_blocks;

    // Module-scoped symbol tables
    std::pmr::unordered_map<std::pmr::string, MirGlobalVar *> m_globals;
    std::pmr::unordered_map<std::pmr::string, MirFunction *> m_functions;

    // Worklist of forward references to patch
    std::pmr::vector<UnresolvedReference> m_pendingFixups;

    size_t m_errorCount{ 0 };
};

} // namespace EzMir

#endif // EZMIR_MIR_PARSER_CONTEXT_H
