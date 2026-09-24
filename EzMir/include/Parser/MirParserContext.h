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

/**
 * Transparent hash over pmr::string keys so symbol tables can be probed with a string_view
 * without materializing a temporary pmr::string key before every lookup.
 */
struct PmrStringHash
{
    using is_transparent = void;

    size_t operator()(std::string_view value) const noexcept { return std::hash<std::string_view>{}(value); }
    size_t operator()(const std::pmr::string &value) const noexcept { return operator()(std::string_view(value)); }
};

/**
 * Transparent equality matching PmrStringHash, comparing the underlying character sequences.
 */
struct PmrStringEqual
{
    using is_transparent = void;

    bool operator()(std::string_view lhs, std::string_view rhs) const noexcept { return lhs == rhs; }
};

/**
 * Arena-backed string map with transparent string_view lookups.
 */
template <typename TValue> using PmrStringMap = std::pmr::unordered_map<std::pmr::string, TValue, PmrStringHash, PmrStringEqual>;

/**
 * Namespaces in which a textual symbol name can be resolved.
 */
enum class SymbolKind
{
    Register,         ///< A local virtual/physical register.
    BasicBlock,       ///< A basic block label within the current function.
    Function,         ///< A module-level function.
    GlobalVar,        ///< A module-level global variable.
    GlobalOrFunction  ///< A module-level symbol that could be either a function or global variable.
};

/**
 * Deferred fixup for an operand that referenced a not-yet-declared symbol.
 */
struct UnresolvedReference
{
    std::pmr::string m_symbolName;                  // Name that has to be resolved later.
    SourceReference *m_ref{ nullptr };              // Source location of the reference for diagnostics.
    MirInstruction *m_targetInstruction{ nullptr }; // Instruction whose operand needs patching.
    size_t m_operandIndex{ 0 };                     // Operand slot to patch within the instruction.
    SymbolKind m_kind{ SymbolKind::BasicBlock };    // Namespace the name belongs to.

    /**
     * Captures a forward reference, copying the name into the supplied arena.
     */
    UnresolvedReference(std::string_view name,
                        MirInstruction *inst,
                        size_t opIdx,
                        SymbolKind kind,
                        SourceReference *ref,
                        std::pmr::memory_resource *mr) :
        m_symbolName(name, mr), m_ref(ref), m_targetInstruction(inst), m_operandIndex(opIdx), m_kind(kind)
    {
    }
};

/**
 * Symbol resolution table, scope tracker, and forward reference patcher for MIR parsing.
 */
class MirParserContext
{
  public:
    /**
     * Binds the resolution tables to the builder context, diagnostics, arena and optional source manager.
     */
    MirParserContext(MirBuilderContext *bCtx,
                     DiagnosticCollector *diagCollector,
                     std::pmr::memory_resource *arena,
                     GenericSourceManager *sourceMgr = nullptr,
                     size_t sourceId = 0);

    // Scoping
    /**
     * Enters a function scope, making its symbols current.
     */
    void enterFunction(MirFunction *func);
    /**
     * Leaves the current function scope and clears its function-scoped tables.
     * Returns false if any block/register validation errors occurred.
     */
    bool exitFunction();

    // Instruction forward-reference binding
    /**
     * Marks the start of a new instruction statement so fixups queued during its operand parsing
     * can be bound to the constructed instruction.
     */
    void beginInstruction();
    /**
     * Attaches the newly constructed instruction to all forward reference fixups queued since beginInstruction().
     */
    void bindInstruction(MirInstruction *inst);
    /**
     * Cancels any uncommitted forward references queued for the current instruction upon parse error.
     */
    void cancelInstruction();

    // Source reference creation helper
    /**
     * Creates a source reference for the given offset/length in this context's source file.
     */
    SourceReference *createRef(size_t offset, size_t length);

    // Type resolution
    /**
     * Maps a parsed AST type node to the corresponding arena-allocated MirType.
     */
    MirType *resolveType(const Ast::MirAstType *astType);

    // Register mapping
    /**
     * Declares a named register in the current function scope, reporting a duplicate if already present.
     */
    MirRegister *
    declareRegister(std::string_view name, MirType *type, SourceReference *ref, MirRegisterClass *regClass = nullptr);
    /**
     * Resolves a register name to its declaration, reporting an error when undefined.
     */
    MirRegister *resolveRegister(std::string_view name, SourceReference *ref);
    /**
     * Returns the named register, declaring it on first use so forward references remain valid.
     */
    MirRegister *getOrCreateRegister(std::string_view name,
                                     MirType *type,
                                     SourceReference *ref,
                                     MirRegisterClass *regClass = nullptr);

    // Basic block mapping & forward references
    /**
     * Declares a named basic block in the current function, rejecting duplicates.
     */
    MirBlock *declareBlock(std::string_view name, SourceReference *ref);
    /**
     * Returns the named block, creating a placeholder on first use to support forward branches.
     */
    MirBlock *getOrCreateBlock(std::string_view name, SourceReference *ref);
    /**
     * Resolves a block label to its declaration, reporting an error when unknown.
     */
    MirBlock *resolveBlock(std::string_view name, SourceReference *ref);

    // Globals & Functions
    /**
     * Registers a global variable under its name in the module scope.
     */
    MirGlobalVar *declareGlobal(std::string_view name, MirGlobalVar *var);
    /**
     * Resolves a global variable name, reporting an error when undefined.
     */
    MirGlobalVar *resolveGlobal(std::string_view name, SourceReference *ref);

    /**
     * Registers a function under its name in the module scope.
     */
    MirFunction *declareFunction(std::string_view name, MirFunction *func);
    /**
     * Resolves a function name, reporting an error when undefined.
     */
    MirFunction *resolveFunction(std::string_view name, SourceReference *ref);

    // Forward reference recording & resolution
    /**
     * Queues a reference to a symbol that was not yet declared so it can be patched once declared.
     */
    void recordForwardReference(
            std::string_view name, MirInstruction *inst, size_t operandIdx, SymbolKind kind, SourceReference *ref);
    /**
     * Resolves queued function references, returning true when all were patched successfully.
     */
    bool resolvePendingFunctionFixups();
    /**
     * Resolves all queued forward references of every kind, returning true on complete success.
     */
    bool resolveAllPendingFixups();

    // SSA and register definition tracking
    /**
     * Records a virtual register definition for the current function.
     * If verifySsa is true and the register was already defined, emits an error with a note
     * referencing the previous definition, records a parse error, and returns false.
     */
    bool recordRegisterDef(std::string_view name, SourceReference *ref, bool verifySsa);
    /**
     * Returns true if the named virtual register has been defined in the current function scope.
     */
    bool hasRegisterDef(std::string_view name) const;
    /**
     * Returns the source reference where the register was defined, or nullptr.
     */
    SourceReference *getRegisterDefRef(std::string_view name) const;
    /**
     * Records a use of a register for use-before-def tracking.
     */
    void recordRegisterUse(std::string_view name, SourceReference *ref);

    // Block definition tracking
    /**
     * Marks a basic block as explicitly defined by a label: declaration in the function body.
     */
    void markBlockDefined(std::string_view name, SourceReference *ref);
    /**
     * Marks a basic block as referenced by a branch target or phi edge.
     */
    void markBlockReferenced(std::string_view name, SourceReference *ref);
    /**
     * Returns true if the named block has been explicitly defined with a label.
     */
    bool isBlockDefined(std::string_view name) const;

    /**
     * Increments the parse error counter.
     */
    void recordError() { ++m_errorCount; }
    /**
     * Returns true when at least one parse error has been recorded.
     */
    bool hasErrors() const { return m_errorCount > 0; }
    /**
     * Returns the number of parse errors recorded so far.
     */
    size_t getErrorCount() const { return m_errorCount; }

    /**
     * Returns the underlying MIR builder context.
     */
    MirBuilderContext *getBuilderContext() const { return m_bCtx; }
    /**
     * Returns the diagnostic collector used for parser errors.
     */
    DiagnosticCollector *getDiagCollector() const { return m_diag; }
    /**
     * Returns the arena that owns parser-created objects.
     */
    std::pmr::memory_resource *getArena() const { return m_arena; }
    /**
     * Returns the function currently being parsed, or nullptr at module scope.
     */
    MirFunction *getCurrentFunction() const { return m_currentFunction; }
    /**
     * Returns the optional source manager used to create source references.
     */
    GenericSourceManager *getSourceManager() const { return m_sourceMgr; }
    /**
     * Returns the numeric ID of the source file being parsed.
     */
    size_t getSourceId() const { return m_sourceId; }

  private:
    /**
     * Materializes a register from its textual name: names of the form "p<digits>" or "%p<digits>"
     * become physical registers bound to that hardware ID, every other name becomes a virtual
     * register. The type defaults to i64 when null.
     */
    MirRegister *materializeRegister(std::string_view name,
                                     MirType *type,
                                     SourceReference *ref,
                                     MirRegisterClass *regClass);

    MirBuilderContext *m_bCtx{ nullptr };          // Builder context receiving constructed MIR entities.
    DiagnosticCollector *m_diag{ nullptr };        // Collector for parser error diagnostics.
    std::pmr::memory_resource *m_arena{ nullptr }; // Arena owning parser-created strings/containers.
    GenericSourceManager *m_sourceMgr{ nullptr };  // Optional source manager for SourceReference creation.
    size_t m_sourceId{ 0 };                        // Source file ID used for created references.
    MirFunction *m_currentFunction{ nullptr };     // Function scope currently being populated.

    // Function-scoped symbol tables
    PmrStringMap<MirRegister *> m_registers; // Name -> declared register.
    PmrStringMap<MirBlock *> m_blocks;       // Label -> declared block.

    // Module-scoped symbol tables
    PmrStringMap<MirGlobalVar *> m_globals;  // Name -> global variable.
    PmrStringMap<MirFunction *> m_functions; // Name -> function.

    // Worklist of forward references to patch
    std::pmr::vector<UnresolvedReference> m_pendingFixups; // Queued unresolved operand references.
    std::pmr::vector<size_t> m_currentInstructionFixups;   // Indices in m_pendingFixups for current instruction.

    // Block tracking for definition-before-reference validation
    PmrStringMap<SourceReference *> m_definedBlocks;
    PmrStringMap<SourceReference *> m_referencedBlocks;

    // Register tracking for SSA validation and use-before-def checks
    PmrStringMap<SourceReference *> m_regDefs;
    PmrStringMap<SourceReference *> m_regUses;

    size_t m_errorCount{ 0 }; // Number of errors reported during parsing.
};

} // namespace EzMir

#endif // EZMIR_MIR_PARSER_CONTEXT_H
