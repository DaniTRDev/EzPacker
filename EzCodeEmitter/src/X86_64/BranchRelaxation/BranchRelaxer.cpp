#include "X86_64/BranchRelaxation/BranchRelaxer.h"

namespace EzCodeEmitter::X86_64
{

void BranchRelaxer::emitBytes(const uint8_t *data, size_t size)
{
    if (data && size > 0)
    {
        m_items.push_back(StreamItem::Data(data, size));
    }
}

void BranchRelaxer::emitBytes(const std::vector<uint8_t> &bytes) { emitBytes(bytes.data(), bytes.size()); }

void BranchRelaxer::defineLabel(MirId labelId) { m_items.push_back(StreamItem::Label(labelId)); }

void BranchRelaxer::emitJmp(MirId targetLabelId) { m_items.push_back(StreamItem::Jmp(targetLabelId)); }

void BranchRelaxer::emitJcc(ConditionCode cc, MirId targetLabelId)
{
    m_items.push_back(StreamItem::Jcc(cc, targetLabelId));
}

void BranchRelaxer::clear() { m_items.clear(); }

size_t BranchRelaxer::relaxAndResolve(std::vector<uint8_t> &outCode,
                                      std::unordered_map<MirId, uint64_t> &resolvedLabels)
{
    size_t relaxedCount = 0;
    bool changed = true;
    std::unordered_map<MirId, uint64_t> labelOffsets;

    // Iterative relaxation loop until all displacements fit within their encoded ranges
    while (changed)
    {
        changed = false;
        labelOffsets.clear();

        // Pass 1: Compute tentative offsets of all labels and instructions
        uint64_t currentOffset = 0;
        for (const auto &item : m_items)
        {
            switch (item.m_kind)
            {
                case StreamItemKind::RawData:
                    currentOffset += item.m_rawData.size();
                    break;
                case StreamItemKind::LabelDef:
                    labelOffsets[item.m_labelId] = currentOffset;
                    break;
                case StreamItemKind::Branch:
                    currentOffset += item.m_branch.getSize();
                    break;
            }
        }

        // Pass 2: Check each branch to see if displacement exceeds signed 8-bit range [-128, +127]
        currentOffset = 0;
        for (auto &item : m_items)
        {
            if (item.m_kind == StreamItemKind::Branch)
            {
                size_t branchLen = item.m_branch.getSize();
                uint64_t nextIp = currentOffset + branchLen;

                auto it = labelOffsets.find(item.m_branch.m_targetId);
                if (it != labelOffsets.end())
                {
                    uint64_t targetOffset = it->second;
                    int64_t disp = static_cast<int64_t>(targetOffset) - static_cast<int64_t>(nextIp);

                    if (!item.m_branch.m_isRelaxed && (disp < -128 || disp > 127))
                    {
                        item.m_branch.m_isRelaxed = true;
                        changed = true;
                        relaxedCount++;
                    }
                }
                currentOffset += item.m_branch.getSize();
            }
            else if (item.m_kind == StreamItemKind::RawData)
            {
                currentOffset += item.m_rawData.size();
            }
        }
    }

    // Final offset computation
    resolvedLabels.clear();
    uint64_t currentOffset = 0;
    for (const auto &item : m_items)
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

    // Pass 3: Emit final linearized byte stream
    outCode.clear();
    currentOffset = 0;
    for (const auto &item : m_items)
    {
        switch (item.m_kind)
        {
            case StreamItemKind::RawData:
                outCode.insert(outCode.end(), item.m_rawData.begin(), item.m_rawData.end());
                currentOffset += item.m_rawData.size();
                break;
            case StreamItemKind::LabelDef:
                // No bytes emitted for label definition marker
                break;
            case StreamItemKind::Branch:
            {
                uint64_t targetOffset = resolvedLabels[item.m_branch.m_targetId];
                size_t branchLen = item.m_branch.getSize();
                uint64_t nextIp = currentOffset + branchLen;
                int64_t disp = static_cast<int64_t>(targetOffset) - static_cast<int64_t>(nextIp);

                if (!item.m_branch.m_isRelaxed)
                {
                    // Short branch (2 bytes)
                    if (item.m_branch.m_isConditional)
                    {
                        InstructionEncoder::emitJccShort(outCode, item.m_branch.m_condition, static_cast<int8_t>(disp));
                    }
                    else
                    {
                        InstructionEncoder::emitJmpShort(outCode, static_cast<int8_t>(disp));
                    }
                }
                else
                {
                    // Near branch (5 or 6 bytes)
                    if (item.m_branch.m_isConditional)
                    {
                        InstructionEncoder::emitJccNear(outCode, item.m_branch.m_condition, static_cast<int32_t>(disp));
                    }
                    else
                    {
                        InstructionEncoder::emitJmpNear(outCode, static_cast<int32_t>(disp));
                    }
                }
                currentOffset += branchLen;
                break;
            }
        }
    }

    return relaxedCount;
}

} // namespace EzCodeEmitter::X86_64
