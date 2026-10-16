#include "RiscV64BranchRelaxer.h"
#include <stdexcept>

namespace EzTargets::RiscV64
{

void RiscV64BranchRelaxer::emitBytes(const uint8_t *data, size_t size)
{
    if (data && size > 0)
    {
        m_items.push_back(StreamItem::Data(data, size));
    }
}

void RiscV64BranchRelaxer::emitBytes(const std::vector<uint8_t> &bytes)
{
    if (!bytes.empty())
    {
        emitBytes(bytes.data(), bytes.size());
    }
}

void RiscV64BranchRelaxer::defineLabel(MirId labelId)
{
    m_items.push_back(StreamItem::Label(labelId));
}

void RiscV64BranchRelaxer::emitJal(uint8_t rd, MirId targetLabelId)
{
    m_items.push_back(StreamItem::Jal(rd, targetLabelId));
}

void RiscV64BranchRelaxer::emitBranch(uint8_t funct3, uint8_t rs1, uint8_t rs2, MirId targetLabelId)
{
    m_items.push_back(StreamItem::Branch(funct3, rs1, rs2, targetLabelId));
}

void RiscV64BranchRelaxer::clear()
{
    m_items.clear();
}

size_t RiscV64BranchRelaxer::relaxAndResolve(std::vector<uint8_t> &outCode,
                                             std::unordered_map<MirId, uint64_t> &resolvedLabels)
{
    size_t relaxedCount = 0;
    bool changed = true;

    while (changed)
    {
        changed = false;
        resolvedLabels.clear();
        uint64_t currentOffset = 0;

        // Pass 1: compute offsets
        for (auto &item : m_items)
        {
            switch (item.m_kind)
            {
                case StreamItemKind::RawData:
                    currentOffset += item.m_rawData.size();
                    break;
                case StreamItemKind::LabelDef:
                    resolvedLabels[item.m_labelId] = currentOffset;
                    break;
                case StreamItemKind::Branch:
                    currentOffset += item.m_branch.getSize();
                    break;
            }
        }

        // Pass 2: check ranges for branches
        currentOffset = 0;
        for (auto &item : m_items)
        {
            if (item.m_kind == StreamItemKind::Branch && item.m_branch.m_isConditional && !item.m_branch.m_isRelaxed)
            {
                auto it = resolvedLabels.find(item.m_branch.m_targetId);
                if (it != resolvedLabels.end())
                {
                    int64_t disp = static_cast<int64_t>(it->second) - static_cast<int64_t>(currentOffset);
                    // B-type range is [-4096, +4094]
                    if (disp < -4096 || disp > 4094)
                    {
                        item.m_branch.m_isRelaxed = true;
                        changed = true;
                        relaxedCount++;
                    }
                }
            }

            if (item.m_kind == StreamItemKind::RawData)
                currentOffset += item.m_rawData.size();
            else if (item.m_kind == StreamItemKind::Branch)
                currentOffset += item.m_branch.getSize();
        }
    }

    // Final pass: emit code
    outCode.clear();
    uint64_t currentOffset = 0;
    for (const auto &item : m_items)
    {
        switch (item.m_kind)
        {
            case StreamItemKind::RawData:
                outCode.insert(outCode.end(), item.m_rawData.begin(), item.m_rawData.end());
                currentOffset += item.m_rawData.size();
                break;
            case StreamItemKind::LabelDef:
                break;
            case StreamItemKind::Branch:
            {
                auto it = resolvedLabels.find(item.m_branch.m_targetId);
                int32_t targetOffset = 0;
                if (it != resolvedLabels.end())
                {
                    targetOffset = static_cast<int32_t>(static_cast<int64_t>(it->second) - static_cast<int64_t>(currentOffset));
                }

                if (!item.m_branch.m_isConditional)
                {
                    InstructionEncoder::emitJal(outCode, item.m_branch.m_rd, targetOffset);
                }
                else if (item.m_branch.m_isRelaxed)
                {
                    // Relaxed: Invert branch to skip over JAL (+8 bytes)
                    uint8_t invFunct3 = item.m_branch.m_funct3 ^ 1;
                    InstructionEncoder::emitBranch(outCode, invFunct3, item.m_branch.m_rs1, item.m_branch.m_rs2, 8);
                    // Emit JAL x0, targetOffset - 4 (since JAL is at currentOffset + 4)
                    InstructionEncoder::emitJal(outCode, 0, targetOffset - 4);
                }
                else
                {
                    InstructionEncoder::emitBranch(outCode, item.m_branch.m_funct3, item.m_branch.m_rs1, item.m_branch.m_rs2, targetOffset);
                }

                currentOffset += item.m_branch.getSize();
                break;
            }
        }
    }

    return relaxedCount;
}

} // namespace EzTargets::RiscV64
