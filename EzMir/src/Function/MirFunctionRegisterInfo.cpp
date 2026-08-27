#include "Function/MirFunctionRegisterInfo.h"

MirFunctionRegisterInfo::MirFunctionRegisterInfo(std::pmr::memory_resource *alloc) : m_vregs(alloc) {}

bool MirFunctionRegisterInfo::hasOneUse(MirId regId) const { return getUseCount(regId) == 1; }

class MirInstruction *MirFunctionRegisterInfo::getDef(MirId regId) const
{
    auto it = m_vregs.find(regId);
    if (it == m_vregs.end())
        return nullptr;

    return it->second.m_defInst;
}

size_t MirFunctionRegisterInfo::getUseCount(MirId regId) const
{
    auto it = m_vregs.find(regId);
    if (it == m_vregs.end())
        return 0;

    return it->second.m_uses.size();
}

void MirFunctionRegisterInfo::clearDef(MirId regId)
{
    auto it = m_vregs.find(regId);
    if (it == m_vregs.end())
        return;

    it->second.m_defInst = nullptr;
}

void MirFunctionRegisterInfo::recordDef(MirId regId, class MirInstruction *inst)
{
    auto it = m_vregs.find(regId);
    if (it == m_vregs.end())
    {
        it = m_vregs.emplace(regId, MirVRegData(m_vregs.get_allocator().resource())).first;
    }

    it->second.m_defInst = inst;
}

void MirFunctionRegisterInfo::recordUse(MirId regId, class MirInstruction *inst, size_t opIndex)
{
    auto it = m_vregs.find(regId);
    if (it == m_vregs.end())
    {
        it = m_vregs.emplace(regId, MirVRegData(m_vregs.get_allocator().resource())).first;
    }

    it->second.m_uses.push_back({ inst, opIndex });
}

void MirFunctionRegisterInfo::removeUse(MirId regId, const class MirInstruction *inst)
{
    auto it = m_vregs.find(regId);
    if (it == m_vregs.end())
        return;

    auto &uses = it->second.m_uses;
    std::erase_if(uses, [inst](const MirVRegUse &u) { return u.m_userInst == inst; });
}

void MirFunctionRegisterInfo::reset() { m_vregs.clear(); }

std::optional<const std::pmr::vector<MirVRegUse>> MirFunctionRegisterInfo::getUses(MirId regId) const
{
    auto it = m_vregs.find(regId);
    if (it == m_vregs.end())
        return std::nullopt;

    return it->second.m_uses;
}