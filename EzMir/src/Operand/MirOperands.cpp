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
std::string MirFloat::toString() const
{
    std::string_view typeName = getMirType() ? getMirType()->getName() : "f64";
    return std::format("{} {}", typeName, m_float.toString(10));
}

/**
 * Initializes an integer constant operand with its MIR type, multi-precision integer value, and source ref.
 */
MirInteger::MirInteger(MirType *type, FlexInt value, SourceReference *ref) : MirOperand(type, ref), m_int(value) {}

/**
 * Formats the integer operand into decimal (0-9) or hexadecimal representation.
 */
std::string MirInteger::toString() const
{
    std::string_view typeName = getMirType() ? getMirType()->getName() : "i64";
    int64_t val = m_int.getI64();
    if (val >= 0 && val <= 9)
    {
        return std::format("{} {}", typeName, val);
    }
    return std::format("{} 0x{:X}", typeName, val);
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
    std::string_view typeStr = getMirType() ? getMirType()->getName() : "i64";

    if (!m_name.empty())
    {
        return std::format("{} %{}{}({}:{})", typeStr, prefix, m_ref.getId(), m_name, className);
    }
    return std::format("{} %{}{}({})", typeStr, prefix, m_ref.getId(), className);
}

/**
 * Initializes a memory operand [base + index*scale + displ] with pointer type and source reference.
 */
MirMemory::MirMemory(
        MirType *type, MirRegister *base, MirInteger *displ, MirRegister *index, uint8_t scale, SourceReference *ref) :
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

/**
 * Shared operand formatter. Parseable mode emits round-trippable .mir syntax; otherwise the
 * operand's detailed diagnostic rendering is used.
 */
std::string formatOperand(MirOperand *operand, bool parseable)
{
    if (!operand)
        return "<null>";

    if (!parseable)
        return operand->toString();

    switch (operand->getType())
    {
        case MirOperandType::Register:
        {
            auto *reg = static_cast<MirRegister *>(operand);
            std::string typeStr = reg->getMirType() ? std::string(reg->getMirType()->getName()) : "i64";
            if (reg->isVirtual())
            {
                std::string name =
                        !reg->getName().empty() ? std::string(reg->getName()) : std::format("%v{}", reg->getRegId());
                if (!name.starts_with("%"))
                    name = "%" + name;
                return std::format("{} {}", typeStr, name);
            }
            const char *className = reg->getRegClass() ? reg->getRegClass()->getName() : "unassigned";
            return std::format("{} %p{}({})", typeStr, reg->getRegId(), className);
        }
        case MirOperandType::Integer:
        {
            auto *imm = static_cast<MirInteger *>(operand);
            std::string typeStr = imm->getMirType() ? std::string(imm->getMirType()->getName()) : "i64";
            return std::format("{} {}", typeStr, imm->getValue().getI64());
        }
        case MirOperandType::FloatingPoint:
        {
            auto *fImm = static_cast<MirFloat *>(operand);
            std::string typeStr = fImm->getMirType() ? std::string(fImm->getMirType()->getName()) : "f64";
            return std::format("{} {}", typeStr, fImm->getValue().toString(10));
        }
        case MirOperandType::Reference:
        {
            auto *ref = static_cast<MirReference *>(operand);
            if (ref->isBlock())
            {
                return std::format("label %block_{}", ref->getRefId());
            }
            else if (ref->isGlobalVar())
            {
                std::string typeStr = ref->getMirType() ? std::string(ref->getMirType()->getName()) : "ptr";
                if (ref->getOffset() > 0)
                {
                    return std::format("{} @global_{}+0x{:X}", typeStr, ref->getRefId(), ref->getOffset());
                }
                return std::format("{} @global_{}", typeStr, ref->getRefId());
            }
            else if (ref->isFunction())
            {
                return std::format("@func_{}", ref->getRefId());
            }
            else if (ref->isStackFrameObject())
            {
                return std::format("%stack[{}]", ref->getRefId());
            }
            return "<invalid_ref>";
        }
        case MirOperandType::RuntimeSymbol:
        {
            auto *rt = static_cast<MirRuntimeSymbol *>(operand);
            if (rt->getSymbolName().starts_with("@"))
            {
                return std::string(rt->getSymbolName());
            }
            return std::format("@{}", rt->getSymbolName());
        }
        case MirOperandType::Memory:
        {
            auto *mem = static_cast<MirMemory *>(operand);
            std::string baseStr = mem->getBase()
                    ? (!mem->getBase()->getName().empty() ? std::string(mem->getBase()->getName())
                                                          : std::format("%v{}", mem->getBase()->getRegId()))
                    : "%0";
            if (!baseStr.starts_with("%"))
                baseStr = "%" + baseStr;

            std::string addrStr = std::format("ptr {}", baseStr);
            if (mem->getIndex())
            {
                std::string idxStr = !mem->getIndex()->getName().empty()
                        ? std::string(mem->getIndex()->getName())
                        : std::format("%v{}", mem->getIndex()->getRegId());
                if (!idxStr.starts_with("%"))
                    idxStr = "%" + idxStr;
                if (mem->getScale() > 1)
                {
                    addrStr += std::format(" + {} * {}", idxStr, mem->getScale());
                }
                else
                {
                    addrStr += std::format(" + {}", idxStr);
                }
            }
            if (mem->getDisplacement() && !mem->getDisplacement()->getValue().isZero())
            {
                int64_t disp = mem->getDisplacement()->getValue().getI64();
                if (disp >= 0)
                {
                    addrStr += std::format(" + {}", disp);
                }
                else
                {
                    addrStr += std::format(" - {}", -disp);
                }
            }
            return std::format("[{}]", addrStr);
        }
        default:
            return operand->toString();
    }
}