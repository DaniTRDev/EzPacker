#include "Operand/MirOperands.h"
#include "Operand/MirRegisterClass.h"
#include "Type/MirType.h"

/**
 * Initializes a floating-point constant operand with its MIR type, multi-precision float value, and source ref.
 */
MirFloat::MirFloat(MirType *type, FlexFloat value, SourceReference *ref) :
    MirOperand(type, ref), m_float(std::move(value))
{
}

/**
 * Formats the float operand into string representation "<type> <decimal-value>".
 */
std::string MirFloat::toString() const { return std::format("{} {}", getMirType()->getName(), m_float.toString(10)); }

/**
 * Initializes an integer constant operand with its MIR type, multi-precision integer value, and source ref.
 */
MirInteger::MirInteger(MirType *type, FlexInt value, SourceReference *ref) : MirOperand(type, ref), m_int(value) {}

/**
 * Formats the integer operand into decimal (0-9) or hexadecimal representation.
 */
std::string MirInteger::toString() const
{
    int64_t val = m_int.getI64();
    if (val >= 0 && val <= 9)
    {
        return std::format("{} {}", getMirType()->getName(), val);
    }
    return std::format("{} 0x{:X}", getMirType()->getName(), val);
}

/**
 * Initializes a symbolic reference operand pointing to a block, function, global, or stack slot.
 */
MirReference::MirReference(MirType *type, MirReferenceType refType, size_t refId, size_t offset, SourceReference *ref) :
    MirOperand(type, ref), m_refType(refType), m_refId(refId), m_offset(offset)
{
}

/**
 * Formats the symbolic reference based on reference target type (block label, global symbol, function, or stack index).
 */
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
        case MirReferenceType::StackFrameObject:
            return std::format("{} %stack[{}]", typePrefix, m_refId);
        default:
            return std::format("{} <invalid_ref>", typePrefix);
    }
}

/**
 * Initializes a runtime library external symbol reference operand with its name and source ref.
 */
MirRuntimeSymbol::MirRuntimeSymbol(MirType *type, std::pmr::string name, SourceReference *ref) :
    MirOperand(type, ref), m_symbolName(std::move(name))
{
}

/**
 * Formats the runtime symbol string representation with leading '@'.
 */
std::string MirRuntimeSymbol::toString() const
{
    if (m_symbolName.starts_with("@"))
        return std::string(m_symbolName);
    return std::format("@{}", m_symbolName);
}

/**
 * Initializes a register operand with its MIR type, virtuality status, ID, source reference, and class.
 */
MirRegister::MirRegister(MirType *type,
                         bool isVirtual,
                         size_t id,
                         SourceReference *ref,
                         MirRegisterClass *_class,
                         std::pmr::string name) :
    MirOperand(type, ref), m_ref(MirRegisterRef(id, isVirtual, _class)), m_name(std::move(name))
{
}

/**
 * Formats the register operand into diagnostic form (e.g. "i32 %v1(unassigned)" or "i64 %p0(rax:GPR64)").
 */
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

/**
 * Initializes a memory operand [base + index*scale + displ] with pointer type and source reference.
 */
MirMemory::MirMemory(MirType *type,
                     MirRegister *base,
                     MirInteger *displ,
                     MirRegister *index,
                     uint8_t scale,
                     SourceReference *ref) :
    MirOperand(type, ref), m_base(base), m_displ(displ), m_index(index), m_scale(scale)
{
}

/**
 * Formats the memory addressing operand as "<type> ptr [<base> + <index>*<scale> +/- <displacement>]".
 */
std::string MirMemory::toString() const
{
    std::pmr::string typePrefix = getMirType() ? getMirType()->getName() : "void";
    std::string addrStr;

    if (m_base)
    {
        addrStr = m_base->toString();
    }

    if (m_index)
    {
        std::string indexStr = m_index->toString();
        if (m_scale > 1)
        {
            indexStr = std::format("{} * {}", indexStr, m_scale);
        }
        if (!addrStr.empty())
        {
            addrStr = std::format("{} + {}", addrStr, indexStr);
        }
        else
        {
            addrStr = indexStr;
        }
    }

    if (addrStr.empty())
    {
        addrStr = "0";
    }

    if (m_displ && !m_displ->getValue().isZero())
    {
        int64_t offset = m_displ->getValue().getI64();
        if (offset >= 0)
        {
            return std::format("{} ptr [{} + 0x{:X}]", typePrefix, addrStr, offset);
        }
        return std::format("{} ptr [{} - 0x{:X}]", typePrefix, addrStr, -offset);
    }

    return std::format("{} ptr [{}]", typePrefix, addrStr);
}