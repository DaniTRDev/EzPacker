#include "Function/MirFunctionRegisterInfo.h"

/**
 * Creates the register bookkeeping map using the supplied arena for its entries.
 */
MirFunctionRegisterInfo::MirFunctionRegisterInfo(std::pmr::memory_resource *alloc) : m_vregs(alloc) {}

/**
 * Returns true when the register has exactly one recorded use.
 */
bool MirFunctionRegisterInfo::hasOneUse(MirId regId) const { return getUseCount(regId) == 1; }

/**
 * Returns the instruction that defines the register, or nullptr if unknown/never defined.
 */
class MirInstruction *MirFunctionRegisterInfo::getDef(MirId regId) const
{
    auto it = m_vregs.find(regId);
    if (it == m_vregs.end())
        return nullptr;

    return it->second.m_defInst;
}

/**
 * Returns how many use sites reference the register; 0 for unknown registers.
 */
size_t MirFunctionRegisterInfo::getUseCount(MirId regId) const
{
    auto it = m_vregs.find(regId);
    if (it == m_vregs.end())
        return 0;

    return it->second.m_uses.size();
}

/**
 * Clears the recorded defining instruction for the register if it is tracked.
 */
void MirFunctionRegisterInfo::clearDef(MirId regId)
{
    auto it = m_vregs.find(regId);
    if (it == m_vregs.end())
        return;

    it->second.m_defInst = nullptr;
}

/**
 * Records the instruction that defines the register, lazily creating its bookkeeping entry.
 */
void MirFunctionRegisterInfo::recordDef(MirId regId, class MirInstruction *inst)
{
    auto it = m_vregs.find(regId);
    if (it == m_vregs.end())
    {
        it = m_vregs.emplace(regId, MirVRegData(m_vregs.get_allocator().resource())).first;
    }

    it->second.m_defInst = inst;
}

/**
 * Appends a use site (user instruction and operand index), lazily creating the register entry.
 */
void MirFunctionRegisterInfo::recordUse(MirId regId, class MirInstruction *inst, size_t opIndex)
{
    auto it = m_vregs.find(regId);
    if (it == m_vregs.end())
    {
        it = m_vregs.emplace(regId, MirVRegData(m_vregs.get_allocator().resource())).first;
    }

    it->second.m_uses.push_back({ inst, opIndex });
}

/**
 * Erases every recorded use whose user instruction is the given instruction.
 */
void MirFunctionRegisterInfo::removeUse(MirId regId, const class MirInstruction *inst)
{
    auto it = m_vregs.find(regId);
    if (it == m_vregs.end())
        return;

    auto &uses = it->second.m_uses;
    std::erase_if(uses, [inst](const MirVRegUse &u) { return u.m_userInst == inst; });
}

/**
 * Drops all register def/use information, typically before re-running an analysis.
 */
void MirFunctionRegisterInfo::reset() { m_vregs.clear(); }

/**
 * Returns a copy of the register's use list, or nullopt when the register is unknown.
 */
std::optional<const std::pmr::vector<MirVRegUse>> MirFunctionRegisterInfo::getUses(MirId regId) const
{
    auto it = m_vregs.find(regId);
    if (it == m_vregs.end())
        return std::nullopt;

    return it->second.m_uses;
}