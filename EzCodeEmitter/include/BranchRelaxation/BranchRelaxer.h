#ifndef EZPACKER_BRANCH_RELAXER_H
#define EZPACKER_BRANCH_RELAXER_H

#include "EzCodeEmitterCommon.h"
#include "X86_64/X86_64Encoding.h"
#include <unordered_map>
#include <vector>

namespace EzCodeEmitter
{

/**
 * Discriminant for stream items in a branch-relaxation block.
 */
enum class StreamItemKind : uint8_t
{
    RawData,
    LabelDef,
    Branch
};

/**
 * Branch instruction descriptor tracking relaxation state.
 */
struct BranchItem
{
    MirId m_targetId{ MIRID_INVALID };
    bool m_isConditional{ false };
    X86_64::ConditionCode m_condition{ X86_64::ConditionCode::E };
    bool m_isRelaxed{ false }; // false = Short (2 bytes), true = Near (5/6 bytes)

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
    StreamItemKind m_kind{ StreamItemKind::RawData };
    std::vector<uint8_t> m_rawData;
    MirId m_labelId{ MIRID_INVALID };
    BranchItem m_branch;

    static StreamItem Data(const uint8_t *data, size_t size)
    {
        StreamItem it;
        it.m_kind = StreamItemKind::RawData;
        it.m_rawData.assign(data, data + size);
        return it;
    }

    static StreamItem Label(MirId id)
    {
        StreamItem it;
        it.m_kind = StreamItemKind::LabelDef;
        it.m_labelId = id;
        return it;
    }

    static StreamItem Jmp(MirId targetId)
    {
        StreamItem it;
        it.m_kind = StreamItemKind::Branch;
        it.m_branch.m_targetId = targetId;
        it.m_branch.m_isConditional = false;
        it.m_branch.m_isRelaxed = false;
        return it;
    }

    static StreamItem Jcc(X86_64::ConditionCode cc, MirId targetId)
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
    void emitJcc(X86_64::ConditionCode cc, MirId targetLabelId);

    /**
     * Performs iterative branch relaxation until fixed point convergence.
     * Generates exact byte-level output into outCode and records final label offsets.
     * Returns the total count of relaxed branches.
     */
    size_t relaxAndResolve(std::vector<uint8_t> &outCode,
                           std::unordered_map<MirId, uint64_t> &resolvedLabels);

    /**
     * Returns all items currently in the stream.
     */
    const std::vector<StreamItem> &getItems() const { return m_items; }

    /**
     * Clears all stream items.
     */
    void clear();

  private:
    std::vector<StreamItem> m_items;
};

} // namespace EzCodeEmitter

#endif // EZPACKER_BRANCH_RELAXER_H
