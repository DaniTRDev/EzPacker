#ifndef EZTARGETS_RISCV64_BRANCH_RELAXER_H
#define EZTARGETS_RISCV64_BRANCH_RELAXER_H

#include "EzTargetsRiscV64Common.h"
#include "Encoding/RiscV64InstructionEncoder.h"
#include <unordered_map>
#include <vector>

namespace EzTargets::RiscV64
{

enum class StreamItemKind : uint8_t
{
    RawData,
    LabelDef,
    Branch
};

struct BranchItem
{
    MirId m_targetId{ MIRID_INVALID };
    bool m_isConditional{ false };
    uint8_t m_funct3{ 0 };
    uint8_t m_rs1{ 0 };
    uint8_t m_rs2{ 0 };
    uint8_t m_rd{ 0 };
    bool m_isRelaxed{ false }; // false = 4 bytes (B-type or JAL), true = 8 bytes (inverted B-type + JAL)

    size_t getSize() const
    {
        if (!m_isConditional)
        {
            return 4; // JAL rd, offset
        }
        return m_isRelaxed ? 8 : 4; // Relaxed: B<inv> +8, JAL x0, target
    }
};

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

    static StreamItem Jal(uint8_t rd, MirId targetId)
    {
        StreamItem it;
        it.m_kind = StreamItemKind::Branch;
        it.m_branch.m_targetId = targetId;
        it.m_branch.m_isConditional = false;
        it.m_branch.m_rd = rd;
        it.m_branch.m_isRelaxed = false;
        return it;
    }

    static StreamItem Branch(uint8_t funct3, uint8_t rs1, uint8_t rs2, MirId targetId)
    {
        StreamItem it;
        it.m_kind = StreamItemKind::Branch;
        it.m_branch.m_targetId = targetId;
        it.m_branch.m_isConditional = true;
        it.m_branch.m_funct3 = funct3;
        it.m_branch.m_rs1 = rs1;
        it.m_branch.m_rs2 = rs2;
        it.m_branch.m_isRelaxed = false;
        return it;
    }
};

/**
 * Iterative branch relaxer for RISC-V 64.
 * Widens conditional branches (±4KB) into inverted branch + JAL (±1MB) when out of range.
 */
class RiscV64BranchRelaxer
{
  public:
    RiscV64BranchRelaxer() = default;

    void emitBytes(const uint8_t *data, size_t size);
    void emitBytes(const std::vector<uint8_t> &bytes);
    void defineLabel(MirId labelId);
    void emitJal(uint8_t rd, MirId targetLabelId);
    void emitBranch(uint8_t funct3, uint8_t rs1, uint8_t rs2, MirId targetLabelId);

    size_t relaxAndResolve(std::vector<uint8_t> &outCode, std::unordered_map<MirId, uint64_t> &resolvedLabels);
    const std::vector<StreamItem> &getItems() const { return m_items; }
    void clear();

  private:
    std::vector<StreamItem> m_items;
};

} // namespace EzTargets::RiscV64

#endif // EZTARGETS_RISCV64_BRANCH_RELAXER_H
