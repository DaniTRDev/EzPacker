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
constexpr MirId MIRID_INVALID = 0;

struct MirGlobalDataEntry
{
    bool m_isReadOnly;             // True for read-only constants / literal data.
    bool m_uninitialized;          // True when the original payload pointer was null.
    size_t m_entryId;              // Unique MIR ID of this global-data entry.
    size_t m_dataSize;             // Size of the stored byte payload.
    ConstantArray<uint8_t> m_data; // Arena-managed byte storage for the payload.
};

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
     * Binds subsequent instruction creation to `block`. If block is nullptr, emission will stop emitting into a block,
     * instructions WILL BE emitted anyways, but won't be inside any block.
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
     * Returns the block pointed to by `ref`, or `nullptr` if `ref` is not a block reference or if no block with the
     * given ID exists in the context.
     * @return MirBlock *
     */
    MirBlock *getBlockFromRef(const MirReference &ref) const;

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
     * Creates a raw global-data entry.
     *
     * The byte range `[data, data + size)` is copied into context-owned
     * storage. If `data` is `nullptr`, the entry is flagged as uninitialized
     * and the allocated storage is zero-filled.
     *
     * @param data       Source bytes to copy, or `nullptr` for an
     *                   uninitialized/zero-filled entry.
     * @param size       Number of bytes to allocate and store.
     * @param isReadOnly Whether the entry should be treated as read-only.
     */
    MirGlobalDataEntry *createGlobalData(const void *data, size_t size, bool isReadOnly = true);

    /**
     * Convenience wrapper that emits a read-only `double` constant.
     */
    MirGlobalDataEntry *createGlobalFloatingPoint(double val);

    /**
     * Convenience wrapper that emits a read-only `uint64_t` constant.
     */
    MirGlobalDataEntry *createGlobalInteger(uint64_t val);

    /**
     * Emits a string as a global byte array.
     *
     * The string contents are copied into context-owned storage. When
     * `includeNullTerminator` is true, one extra `\0` byte is appended after
     * the copied characters.
     */
    MirGlobalDataEntry *
    createGlobalString(const std::string_view &str, bool includeNullTerminator = true, bool isReadOnly = true);

    /**
     * Returns the global data entry of the given entry ID.
     * @param entryId
     * @return
     */
    MirGlobalDataEntry *getGlobalDataEntryFromId(size_t entryId);

    /**
     * Creates a new `MirReference` that points to `block`.
     * @param block
     * @return MirReference
     */
    MirReference createReference(MirBlock *block);

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

    /**
     * Returns the type list defined in this context.
     */
    TypedPoolSlice<MirType> *getTypeList() const;

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
    std::map<size_t, MirBlock *> m_idToBlockMap; // Fast lookup from MIR block ID to MirBlock.
    std::map<size_t, MirGlobalDataEntry *> m_idToGlobalDataEntry;
    std::set<std::string_view> m_typeNames; // Set used to search, by name, if a type exists. Sets allows log n lookup.
};

#endif // EZPACKER_MIREMITTERCONTEXT_H
