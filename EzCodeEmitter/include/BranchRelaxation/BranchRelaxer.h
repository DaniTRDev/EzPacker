#ifndef EZPACKER_BRANCH_RELAXER_H
#define EZPACKER_BRANCH_RELAXER_H

#include "EzCodeEmitterCommon.h"
#include "TableGen/InstructionEncoder.h"
#include <unordered_map>
#include <vector>

namespace EzCodeEmitter
{

/**
 * Discriminant for stream items in a branch-relaxation block.
 */
enum class StreamItemKind : uint8_t
{
    RawData,  ///< Opaque machine bytes emitted verbatim.
    LabelDef, ///< Label marker anchoring the current stream offset.
    Branch    ///< Relaxable branch that may be widened during resolution.
};

/**
 * Branch instruction descriptor tracking relaxation state.
 */
struct BranchItem
{
    MirId m_targetId{ MIRID_INVALID };                                 ///< Label this branch targets.
    bool m_isConditional{ false };                                     ///< True for Jcc, false for JMP.
    TableGen::ConditionCode m_condition{ TableGen::ConditionCode::E }; ///< Predicate used by conditional branches.
    bool m_isRelaxed{ false }; // false = Short (2 bytes), true = Near (5/6 bytes)

    /**
     * Returns the encoded length of the branch given its current relaxation state.
     */
    size_t getSize() const
    {
        if (!m_isRelaxed)
        {
            return 2; // Short: 0xEB disp8 or 0x7x disp8
        }
        return m_isConditional ? 6 : 5; // Near: 0x0F 0x8x disp32 or 0xE9 disp32
    }
};

/**
 * Item in the linear branch-resolution stream.
 */
struct StreamItem
{
    StreamItemKind m_kind{ StreamItemKind::RawData }; ///< Which payload below is active.
    std::vector<uint8_t> m_rawData;                   ///< Bytes for RawData items.
    MirId m_labelId{ MIRID_INVALID };                 ///< Label id for LabelDef items.
    BranchItem m_branch;                              ///< Branch state for Branch items.

    /**
     * Builds a RawData item by copying the given byte range.
     */
    static StreamItem Data(const uint8_t *data, size_t size)
    {
        StreamItem it;
        it.m_kind = StreamItemKind::RawData;
        it.m_rawData.assign(data, data + size);
        return it;
    }

    /**
     * Builds a LabelDef item that defines id at the current stream offset.
     */
    static StreamItem Label(MirId id)
    {
        StreamItem it;
        it.m_kind = StreamItemKind::LabelDef;
        it.m_labelId = id;
        return it;
    }

    /**
     * Builds an unconditional jump branch item targeting targetId.
     */
    static StreamItem Jmp(MirId targetId)
    {
        StreamItem it;
        it.m_kind = StreamItemKind::Branch;
        it.m_branch.m_targetId = targetId;
        it.m_branch.m_isConditional = false;
        it.m_branch.m_isRelaxed = false;
        return it;
    }

    /**
     * Builds a conditional jump branch item guarded by the given condition code.
     */
    static StreamItem Jcc(TableGen::ConditionCode cc, MirId targetId)
    {
        StreamItem it;
        it.m_kind = StreamItemKind::Branch;
        it.m_branch.m_targetId = targetId;
        it.m_branch.m_isConditional = true;
        it.m_branch.m_condition = cc;
        it.m_branch.m_isRelaxed = false;
        return it;
    }
};

/**
 * Two-pass iterative branch relaxation and label resolution engine.
 * Computes exact branch displacements and expands short 8-bit relative jumps into
 * near 32-bit relative jumps whenever displacements exceed [-128, +127].
 */
class BranchRelaxer
{
  public:
    BranchRelaxer() = default;

    /**
     * Appends raw machine instruction bytes to the block.
     */
    void emitBytes(const uint8_t *data, size_t size);

    /**
     * Appends a vector of bytes.
     */
    void emitBytes(const std::vector<uint8_t> &bytes);

    /**
     * Defines a label at the current stream location.
     */
    void defineLabel(MirId labelId);

    /**
     * Emits an unconditional jump to the target label.
     */
    void emitJmp(MirId targetLabelId);

    /**
     * Emits a conditional jump to the target label.
     */
    void emitJcc(TableGen::ConditionCode cc, MirId targetLabelId);

    /**
     * Performs iterative branch relaxation until fixed point convergence.
     * Generates exact byte-level output into outCode and records final label offsets.
     * Returns the total count of relaxed branches.
     */
    size_t relaxAndResolve(std::vector<uint8_t> &outCode, std::unordered_map<MirId, uint64_t> &resolvedLabels);

    /**
     * Returns all items currently in the stream.
     */
    const std::vector<StreamItem> &getItems() const { return m_items; }

    /**
     * Clears all stream items.
     */
    void clear();

  private:
    std::vector<StreamItem> m_items; ///< Ordered stream of data, labels and branches to resolve.
};

} // namespace EzCodeEmitter

#endif // EZPACKER_BRANCH_RELAXER_H
