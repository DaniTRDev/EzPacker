/**
 * @file MirEmitterContext.h
 * @brief Central bookkeeping for MIR allocation, IDs, active bindings, and arena pools.
 */
#ifndef EZPACKER_MIREMITTERCONTEXT_H
#define EZPACKER_MIREMITTERCONTEXT_H

#include "EzMirCommon.h"
#include "MirBlock.h"
#include "Function/MirFunction.h"
#include "Type/MirType.h"

/**
 * A struct that contains information about global data.
 */
struct MirGlobalDataEntry
{
    bool m_isReadOnly;
    bool m_uninitialized;
    std::pmr::vector<char> m_data;
    MirType *m_dataType;
    size_t m_entryId;
};

/**
 * Insertion mode of an insertstate.
 */
enum class InsertMode : uint8_t
{
    Append,      // Back.
    InsertBefore // Before a point.
};

struct InsertState
{
    MirBlock *block{ nullptr };
    InsertMode mode{ InsertMode::Append };
    std::pmr::list<MirInstruction>::iterator iterator{};
};

class MirEmitterContext
{
  public:
    MirEmitterContext(const std::shared_ptr<DiagnosticCollector> &diagCollector);

    // Disable copy/move constructors to preserve safety across the arena resource references
    MirEmitterContext(const MirEmitterContext &) = delete;
    MirEmitterContext &operator=(const MirEmitterContext &) = delete;

    /**
     * Returns the current insertion point of the context.
     * @return
     */
    const InsertState &getInsertPoint() const;

    /**
     * @brief Returns the block currently bound for instruction emission.
     */
    MirBlock *getCurrentBlock() const;

    /**
     * Creates a block and appends it to the current function, if any.
     * @return
     */
    MirBlock *createBlock();

    /**
     * Creates a function with the name, return type and parameters. This function will also allocate a block
     * inside the function.
     * @param name
     * @param returnType
     * @param params
     * @return
     */
    MirFunction *
    createFunc(const std::string &name, MirType *returnType, const std::initializer_list<MirRegister *> &params);

    /**
     * Creates an ID within this context. Returns 0 (MIRID_INVALID) if failed.
     * @return
     */
    MirId createId();

    /**
     * Creates an instruction at the current insert state. By default the instruction is empty, operands should be
     * pushed by the caller manually.
     * @param opcode
     * @return
     */
    MirInstruction *createInstruction(MirInstructionOpCode opcode);

    /**
     * Creates an instruction at the current insert state. This function will also append the operands of the
     * instruction.
     * @param opcode
     * @return
     */
    MirInstruction *createInstruction(MirInstructionOpCode opcode, const std::initializer_list<MirOperand *> &operands);

    /**
     * @brief Sets the emitter to append instructions to the end of the specified block.
     * @return `false` when `block` is `nullptr`; otherwise `true`.
     */
    void setInsertPoint(MirBlock *block);

    /**
     * @brief Sets the emitter to insert newly created instructions BEFORE the specified iterator.
     * The sequence of emitted instructions will be automatically preserved.
     */
    void setInsertPoint(MirBlock *block, std::pmr::list<MirInstruction>::iterator insertBeforeIt);

    /**
     * Returns an allocator used to allocate complementary resources (global data, types, names, ...).
     * @return
     */
    std::pmr::monotonic_buffer_resource *getGlobalAllocator();

    /**
     * Returns an allocator related to functions (blocks mainly).
     * @return
     */
    std::pmr::monotonic_buffer_resource *getFuncAllocator();

  private:
    MirId m_currentId{ 0 };
    InsertState m_insertState;

    // Pools.
    std::pmr::monotonic_buffer_resource m_globalResource;
    std::pmr::monotonic_buffer_resource m_functionResource;

    std::pmr::list<MirFunction *> m_functions;
    std::pmr::vector<MirGlobalDataEntry *> m_globalData;

    std::shared_ptr<DiagnosticCollector> m_diagCollector;
};

#endif // EZPACKER_MIREMITTERCONTEXT_H