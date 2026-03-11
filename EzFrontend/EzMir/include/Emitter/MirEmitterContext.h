/**
 * @file MirEmitterContext.h
 * @brief Central bookkeeping for MIR allocation, IDs, active bindings, and arena pools.
 *
 * `MirEmitterContext` is the owning hub behind EzMir construction. It stores
 * every arena/pool used to allocate MIR entities and keeps track of the
 * currently active block/function so emitters can append instructions and
 * blocks automatically.
 *
 * Key responsibilities:
 *   - issue monotonic non-zero MIR IDs,
 *   - allocate blocks, instructions, functions, types, and global data,
 *   - remember the currently bound block,
 *   - remember the currently active function so new blocks join it,
 *   - map MIR type IDs back to `MirType*` for later lookup.
 *
 * `MIRID_INVALID` is reserved as the sentinel "no MIR object" value.
 */
#ifndef EZPACKER_MIREMITTERCONTEXT_H
#define EZPACKER_MIREMITTERCONTEXT_H

#include "EzMirCommon.h"
#include "MirBlock.h"
#include "Function/MirFunction.h"
#include "Type/MirType.h"

using MirId = size_t;
constexpr MirId MIRID_INVALID = 0; // Sentinel used for invalid/unlinked MIR IDs.

class MirEmitterContext : public ErrorEmitter
{
  public:
    /**
     * Creates an empty MIR context.
     *
     * IDs start at `1`, no block or function is initially bound, and the root
     * slices for functions and types are created eagerly.
     */
    MirEmitterContext(const std::shared_ptr<ErrorCollector> &errorCollector,
                      const std::shared_ptr<SourceManager> &sourceManager);

    /**
     * Binds subsequent instruction creation to `block`.
     *
     * After a successful bind, `createInstruction()` automatically appends new
     * instructions to the block's instruction slice.
     *
     * @return `false` when `block` is `nullptr`; otherwise `true`.
     */
    bool bindToBlock(MirBlock *block);

    /**
     * Returns `true` if the given type ID exists in the MIR type table.
     */
    bool doesTypeExist(size_t typeId) const;
    
    /**
     * Returns `true` if the given type name exists in the MIR type table.
     */
    bool doesTypeExist(const std::string_view &typeName) const;

    /**
     * Allocates a new empty block.
     *
     * If a function is currently active, the block is also appended to that
     * function's block list. The new block is not automatically bound as the
     * current block; callers must bind it explicitly if they want to emit into
     * it.
     */
    MirBlock *createBlock();

    /**
     * Returns the block currently bound for instruction emission, or `nullptr`
     * if no block is bound.
     */
    MirBlock *getCurrentBoundBlock() const;

    /**
     * Returns a fresh, monotonic, non-zero MIR ID.
     */
    MirId createId();

    /**
     * Creates a new instruction with an empty operand slice.
     *
     * If a block is currently bound, the instruction is appended to that
     * block's instruction list immediately.
     */
    MirInstruction *createInstruction(MirInstructionOpCode opcode);

    /**
     * Creates a new function and makes it the active function in the context.
     *
     * On success the function receives:
     *   - a fresh entry-point block,
     *   - an initially one-element block list containing that entry point,
     *   - an empty parameter list,
     *   - and a fresh function ID.
     *
     * `returnTypeId` must be non-zero, otherwise a fatal error is emitted and
     * `nullptr` is returned.
     */
    MirFunction *createFunction(size_t returnTypeId);

    /**
     * Creates and registers a new MIR type.
     *
     * The created type is appended to the context's type list and inserted into
     * the ID -> type lookup map. An empty `name` is rejected with a fatal
     * diagnostic and results in `nullptr`.
     */
    MirType *createType(MirTypeKind kind, TypedPoolSlice<MirType> *types, const std::string_view &name);

    /**
     * Returns the MIR type for `id`.
     *
     * `id` must be non-zero and present in the context's type map. Otherwise a
     * fatal diagnostic is emitted and `nullptr` is returned.
     */
    MirType *getMirTypeById(size_t id);

    /** Returns the arena pool used to allocate `MirBlock` objects and block slices. */
    TypedPool *getBlockPool();

    /** Returns the arena pool used to allocate `MirGlobalDataEntry` objects. */
    TypedPool *getDataEntryPool();

    /** Returns the arena pool used to allocate `MirFunction` objects and function lists. */
    TypedPool *getFunctionPool();

    /** Returns the arena pool used for function-parameter operand slices. */
    TypedPool *getFunctionParameterPool();

    /** Returns the arena pool used to allocate `MirInstruction` objects and slices. */
    TypedPool *getInstructionPool();

    /** Returns the arena pool used to allocate instruction operands and operand slices. */
    TypedPool *getOperandPool();

    /** Returns the byte-array pool used to store copied global-data payloads. */
    TypedArrayPool<uint8_t> *getEntryDataPool();

    /** Returns the arena pool used to allocate `MirType` objects and type slices. */
    TypedPool *getTypePool();

    /** Returns the list of functions created in this context. */
    TypedPoolSlice<MirFunction> *getFunctionList() const;

  private:
    MirId m_currentId; // Next MIR ID to issue; 0 is reserved as invalid.

    MirBlock *m_currentBoundBlock;
    MirFunction *m_currentBoundFunction;

    TypedPool m_blockPool;
    TypedPool m_dataEntryPool; // Allocates MirGlobalDataEntry objects; raw bytes live in m_dataPool.
    TypedPool m_functionPool;
    TypedPool m_functionParameterPool;
    TypedPool m_instructionPool;
    TypedPool m_operandPool;
    TypedPool m_typePool;
    TypedArrayPool<uint8_t> m_dataPool;          // Stores copied bytes for global data entries.
    TypedPoolSlice<MirFunction> *m_functionList; // Root list of functions created in this context.
    TypedPoolSlice<MirType> *m_typeList;         // Root list of MIR types created in this context.
    std::map<size_t, MirType *> m_idToTypeMap;   // Fast lookup from MIR type ID to MirType.
    std::set<std::string_view> m_typeNames; // Set used to search, by name, if a type exists. Sets allows log n lookup.
};

#endif // EZPACKER_MIREMITTERCONTEXT_H
