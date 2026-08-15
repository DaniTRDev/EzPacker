#ifndef EZPACKER_MIROPERANDS_H
#define EZPACKER_MIROPERANDS_H

#include "EzMirCommon.h"
#include "MirOperand.h"
#include "MirRegisterReference.h"
#include <format>
#include <string>

enum class MirReferenceType : uint8_t
{
    Invalid = 0,
    Block,
    GlobalArrayElem,
    GlobalVar,
    Function,
    ClassField,
    ClassMethod,
    StackFrameObject,
    ConstantArrayElement
};

// =========================================================================
// 1. FLOATING POINT IMMEDIATE
// =========================================================================
class MirFloat : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::FloatingPoint;

    MirFloat(MirType *type, FlexFloat value, SourceReference *ref) : MirOperand(type, ref), m_float(std::move(value)) {}

    FlexFloat &getValue() { return m_float; }
    const FlexFloat &getValue() const { return m_float; }
    MirOperandType getType() const override { return OpKind; }

    std::string toString() const override
    {
        // Output format: f32 3.14159 or f64 0.0
        return std::format("{} {}", getMirType()->getName(), m_float.toString(10));
    }

  private:
    FlexFloat m_float;
};

// =========================================================================
// 2. INTEGER IMMEDIATE
// =========================================================================
class MirInteger : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::Integer;

    MirInteger(MirType *type, FlexInt value, SourceReference *ref) : MirOperand(type, ref), m_int(value) {}

    FlexInt &getValue() { return m_int; }
    const FlexInt &getValue() const { return m_int; }
    MirOperandType getType() const override { return OpKind; }

    std::string toString() const override
    {
        // Output format: i32 1...9 or i64 0x2A for larger values
        int64_t val = m_int.getI64();
        if (val >= 0 && val <= 9)
        {
            return std::format("{} {}", getMirType()->getName(), val);
        }
        return std::format("{} 0x{:X}", getMirType()->getName(), val);
    }

  private:
    FlexInt m_int;
};

// =========================================================================
// 3. CONSTANT ARRAY
// =========================================================================
class MirConstantArray : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::ConstantArray;

    MirConstantArray(MirType *arrayType, std::pmr::vector<MirOperand *> elements, SourceReference *ref) :
        MirOperand(arrayType, ref), m_elements(std::move(elements))
    {
    }

    const std::pmr::vector<MirOperand *> &getElements() const { return m_elements; }
    MirOperandType getType() const override { return OpKind; }

    std::string toString() const override
    {
        // Output format: i32[3] [i32 1, i32 2, i32 3]
        std::string res = std::format("{} [", getMirType()->getName());
        for (size_t i = 0; i < m_elements.size(); ++i)
        {
            if (i > 0)
                res += ", ";
            res += m_elements[i] ? m_elements[i]->toString() : "<null>";
        }
        return res + "]";
    }

  private:
    std::pmr::vector<MirOperand *> m_elements;
};

// =========================================================================
// 4. SYMBOLIC & ABSTRACT REFERENCES
// =========================================================================
class MirReference : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::Reference;

    MirReference(MirType *type, MirReferenceType refType, size_t refId, size_t offset, SourceReference *ref) :
        MirOperand(type, ref), m_refType(refType), m_refId(refId), m_offset(offset)
    {
    }

    bool isBlock() const { return m_refType == MirReferenceType::Block; }
    bool isGlobalArrayElem() const { return m_refType == MirReferenceType::GlobalArrayElem; }
    bool isGlobalVar() const { return m_refType == MirReferenceType::GlobalVar; }
    bool isFunction() const { return m_refType == MirReferenceType::Function; }
    bool isClassField() const { return m_refType == MirReferenceType::ClassField; }
    bool isClassMethod() const { return m_refType == MirReferenceType::ClassMethod; }
    bool isConstantArrayElem() const { return m_refType == MirReferenceType::ConstantArrayElement; }
    bool isStackFrameObject() const { return m_refType == MirReferenceType::StackFrameObject; }
    bool isInvalid() const { return m_refType == MirReferenceType::Invalid; }

    MirReferenceType getRefType() const { return m_refType; }
    MirOperandType getType() const override { return OpKind; }
    size_t getRefId() const { return m_refId; }
    size_t getOffset() const { return m_offset; }

    std::string toString() const override
    {
        auto typePrefix = getMirType() ? getMirType()->getName() : "void";

        switch (m_refType)
        {
            case MirReferenceType::Block:
                return std::format("label %block_{}", m_refId);
            case MirReferenceType::GlobalVar:
                if (m_offset > 0)
                    return std::format("{}* @global_{}+0x{:X}", typePrefix, m_refId, m_offset);
                return std::format("{}* @global_{}", typePrefix, m_refId);
            case MirReferenceType::GlobalArrayElem:
                return std::format("{}* @global_{}[{}]", typePrefix, m_refId, m_offset);
            case MirReferenceType::Function:
                return std::format("{}() @func_{}", typePrefix, m_refId);
            case MirReferenceType::ClassField:
                return std::format("{}* %v{}.field_{}", typePrefix, m_refId, m_offset);
            case MirReferenceType::ClassMethod:
                return std::format("{} %v{}.method_{}", typePrefix, m_refId, m_offset);
            case MirReferenceType::ConstantArrayElement:
                return std::format("{} %v{}[{}]", typePrefix, m_refId, m_offset);
            case MirReferenceType::StackFrameObject:
                return std::format("{}* %stack[{}]", typePrefix, m_refId);
            default:
                return std::format("{} <invalid_ref>", typePrefix);
        }
    }

  private:
    MirReferenceType m_refType;
    size_t m_refId{ 0 };
    size_t m_offset{ 0 };
};

// =========================================================================
// 5. RUNTIME ABI SYMBOL
// =========================================================================
class MirRuntimeSymbol : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::RuntimeSymbol;

    MirRuntimeSymbol(MirType *type, std::pmr::string name, SourceReference *ref) :
        MirOperand(type, ref), m_symbolName(std::move(name))
    {
    }

    MirOperandType getType() const override { return OpKind; }
    const std::pmr::string &getSymbolName() const { return m_symbolName; }

    std::string toString() const override
    {
        // Output format: @__udivti3 or @memcpy
        if (m_symbolName.starts_with("@"))
            return std::string(m_symbolName);
        return std::format("@{}", m_symbolName);
    }

  private:
    std::pmr::string m_symbolName;
};

// =========================================================================
// 6. REGISTER OPERAND
// =========================================================================
class MirRegister : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::Register;

    MirRegister(MirType *type,
                bool isVirtual,
                size_t id,
                SourceReference *ref,
                MirRegisterClass *_class = nullptr,
                std::pmr::string name = "") :
        MirOperand(type, ref), m_ref(RegisterRef(id, isVirtual, _class)), m_name(std::move(name))
    {
    }

    bool isVirtual() const { return m_ref.isVirtual(); }
    bool isPhysical() const { return !m_ref.isVirtual(); }
    bool operator==(const MirRegister &other) const { return m_ref == other.m_ref; }

    MirOperandType getType() const override { return OpKind; }
    RegisterRef getRef() const { return m_ref; }
    size_t getRegId() const { return m_ref.getId(); }

    void setClass(MirRegisterClass *_class) { m_ref.setClass(_class); }
    void setRef(RegisterRef ref) { m_ref = ref; }

    std::string toString() const override
    {
        char prefix = m_ref.isVirtual() ? 'v' : 'p';
        const char *className = m_ref.getClass() ? m_ref.getClass()->getName() : "unassigned";
        std::pmr::string typeStr = getMirType()->getName();

        // Format: i32 %v5(val32:GPR32) or i64 %p4(rsp:GPR64)
        if (!m_name.empty())
        {
            return std::format("{} %{}{}({}:{})", typeStr, prefix, m_ref.getId(), m_name, className);
        }
        return std::format("{} %{}{}({})", typeStr, prefix, m_ref.getId(), className);
    }

    const std::pmr::string &getName() const { return m_name; }

  private:
    RegisterRef m_ref;
    std::pmr::string m_name;
};

// =========================================================================
// 7. FRAME INDEX OPERAND
// =========================================================================
class MirFrameIndex : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::FrameIndex;

    MirFrameIndex(MirType *type, size_t frameId, SourceReference *ref) : MirOperand(type, ref), m_frameId(frameId) {}

    size_t getFrameId() const { return m_frameId; }
    MirOperandType getType() const override { return OpKind; }

    std::string toString() const override
    {
        // Output format: [stack#3]
        return std::format("[stack#{}]", m_frameId);
    }

  private:
    size_t m_frameId{ 0 };
};

// =========================================================================
// 8. CONCRETE MEMORY OPERAND
// =========================================================================
class MirMemory : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::Memory;

    MirMemory(MirType *type, MirRegister *base, MirInteger *displ, SourceReference *ref) :
        MirOperand(type, ref), m_base(base), m_displ(displ)
    {
    }

    MirRegister *getBase() const { return m_base; }
    MirInteger *getDisplacement() const { return m_displ; }
    bool hasBaseReg() const { return m_base != nullptr; }
    MirOperandType getType() const override { return OpKind; }

    std::string toString() const override
    {
        std::string baseStr = m_base ? m_base->toString() : "0";
        std::pmr::string typePrefix = getMirType()->getName();

        if (m_displ && !m_displ->getValue().isZero())
        {
            int64_t offset = m_displ->getValue().getI64();
            if (offset >= 0)
            {
                // Format: i32 ptr [i64 %p4(rsp:GPR64) + 0x10]
                return std::format("{} ptr [{} + 0x{:X}]", typePrefix, baseStr, offset);
            }
            // Format: f64 ptr [i64 %p4(rsp:GPR64) - 0x10]
            return std::format("{} ptr [{} - 0x{:X}]", typePrefix, baseStr, -offset);
        }

        // Format: i32 ptr [i64 %p4(rsp:GPR64)]
        return std::format("{} ptr [{}]", typePrefix, baseStr);
    }

  private:
    MirRegister *m_base;
    MirInteger *m_displ;
};

#endif // EZPACKER_MIROPERANDS_H