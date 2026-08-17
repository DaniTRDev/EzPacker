#include "Operand/MirOperands.h"
#include "Operand/MirRegisterClass.h"
#include "Type/MirType.h"

MirFloat::MirFloat(MirType *type, FlexFloat value, SourceReference *ref) :
    MirOperand(type, ref), m_float(std::move(value))
{
}

std::string MirFloat::toString() const { return std::format("{} {}", getMirType()->getName(), m_float.toString(10)); }

MirInteger::MirInteger(MirType *type, FlexInt value, SourceReference *ref) : MirOperand(type, ref), m_int(value) {}

std::string MirInteger::toString() const
{
    int64_t val = m_int.getI64();
    if (val >= 0 && val <= 9)
    {
        return std::format("{} {}", getMirType()->getName(), val);
    }
    return std::format("{} 0x{:X}", getMirType()->getName(), val);
}

MirReference::MirReference(MirType *type, MirReferenceType refType, size_t refId, size_t offset, SourceReference *ref) :
    MirOperand(type, ref), m_refType(refType), m_refId(refId), m_offset(offset)
{
}

std::string MirReference::toString() const
{
    auto typePrefix = getMirType() ? getMirType()->getName() : "void";

    switch (m_refType)
    {
        case MirReferenceType::Block:
            return std::format("label %block_{}", m_refId);
        case MirReferenceType::GlobalVar:
            if (m_offset > 0)
                return std::format("{} @global_{}+0x{:X}", typePrefix, m_refId, m_offset);
            return std::format("{} @global_{}", typePrefix, m_refId);
        case MirReferenceType::Function:
            return std::format("{}() @func_{}", typePrefix, m_refId);
        case MirReferenceType::ClassField:
            return std::format("{} %v{}.field_{}", typePrefix, m_refId, m_offset);
        case MirReferenceType::ClassMethod:
            return std::format("{} %v{}.method_{}", typePrefix, m_refId, m_offset);
        case MirReferenceType::StackFrameObject:
            return std::format("{} %stack[{}]", typePrefix, m_refId);
        default:
            return std::format("{} <invalid_ref>", typePrefix);
    }
}

MirRuntimeSymbol::MirRuntimeSymbol(MirType *type, std::pmr::string name, SourceReference *ref) :
    MirOperand(type, ref), m_symbolName(std::move(name))
{
}

std::string MirRuntimeSymbol::toString() const
{
    if (m_symbolName.starts_with("@"))
        return std::string(m_symbolName);
    return std::format("@{}", m_symbolName);
}

MirRegister::MirRegister(MirType *type,
                         bool isVirtual,
                         size_t id,
                         SourceReference *ref,
                         MirRegisterClass *_class,
                         std::pmr::string name) :
    MirOperand(type, ref), m_ref(MirRegisterRef(id, isVirtual, _class)), m_name(std::move(name))
{
}

std::string MirRegister::toString() const
{
    char prefix = m_ref.isVirtual() ? 'v' : 'p';
    const char *className = m_ref.getClass() ? m_ref.getClass()->getName() : "unassigned";
    std::pmr::string typeStr = getMirType()->getName();

    if (!m_name.empty())
    {
        return std::format("{} %{}{}({}:{})", typeStr, prefix, m_ref.getId(), m_name, className);
    }
    return std::format("{} %{}{}({})", typeStr, prefix, m_ref.getId(), className);
}

MirMemory::MirMemory(MirType *type, MirRegister *base, MirInteger *displ, SourceReference *ref) :
    MirOperand(type, ref), m_base(base), m_displ(displ)
{
}

std::string MirMemory::toString() const
{
    std::string baseStr = m_base ? m_base->toString() : "0";
    std::pmr::string typePrefix = getMirType()->getName();

    if (m_displ && !m_displ->getValue().isZero())
    {
        int64_t offset = m_displ->getValue().getI64();
        if (offset >= 0)
        {
            return std::format("{} ptr [{} + 0x{:X}]", typePrefix, baseStr, offset);
        }
        return std::format("{} ptr [{} - 0x{:X}]", typePrefix, baseStr, -offset);
    }

    return std::format("{} ptr [{}]", typePrefix, baseStr);
}