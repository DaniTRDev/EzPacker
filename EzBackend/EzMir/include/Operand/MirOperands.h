#ifndef EZPACKER_MIROPERANDS_H
#define EZPACKER_MIROPERANDS_H

#include "EzMirCommon.h"
#include "MirOperand.h"
#include "MirRegisterReference.h"

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

class MirFloat : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::FloatingPoint;

    MirFloat(MirType *type, FlexFloat value, SourceReference *ref) : MirOperand(type, ref), m_float(std::move(value)) {}

    FlexFloat &getValue() { return m_float; }
    MirOperandType getType() const override { return OpKind; }

    std::string toString() const override
    {
        // Output format: f32 3.14159
        return std::format("{} {}", getMirType()->getName(), m_float.toString(10));
    }

  private:
    FlexFloat m_float;
};

class MirInteger : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::Integer;

    MirInteger(MirType *type, FlexInt value, SourceReference *ref) : MirOperand(type, ref), m_int(value) {}

    FlexInt &getValue() { return m_int; }
    MirOperandType getType() const override { return OpKind; }

    std::string toString() const override
    {
        // Output format: i32 0x2A
        return std::format("{} {}", getMirType()->getName(), m_int.toString(16));
    }

  private:
    FlexInt m_int;
};

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
        // Output format: i32[3] [i32 0x1, i32 0x2, i32 0x3]
        std::string res = std::format("{} [", getMirType()->getName());
        for (size_t i = 0; i < m_elements.size(); ++i)
        {
            if (i > 0)
                res += ", ";
            res += m_elements[i]->toString();
        }
        return res + "]";
    }

  private:
    std::pmr::vector<MirOperand *> m_elements;
};

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
        auto typePrefix = getMirType()->getName();

        switch (m_refType)
        {
            case MirReferenceType::Block:
                return std::format("label %block_{}", m_refId);
            case MirReferenceType::GlobalVar:
                return std::format("{} @global_{}+{}", typePrefix, m_refId, m_offset);
            case MirReferenceType::GlobalArrayElem:
                return std::format("{} @global_{}[{}]", typePrefix, m_refId, m_offset);
            case MirReferenceType::Function:
                return std::format("{} @func_{}", typePrefix, m_refId);
            case MirReferenceType::ClassField:
                return std::format("{} %v{}.field_{}", typePrefix, m_refId, m_offset);
            case MirReferenceType::ClassMethod:
                return std::format("{} %v{}.method_{}", typePrefix, m_refId, m_offset);
            case MirReferenceType::ConstantArrayElement:
                return std::format("{} %v{}[{}]", typePrefix, m_refId, m_offset);
            case MirReferenceType::StackFrameObject:
                return std::format("{} %stack[{}]", typePrefix, m_refId);
            default:
                return std::format("{} <invalid_ref>", typePrefix);
        }
    }

  private:
    MirReferenceType m_refType;
    size_t m_refId{ 0 };
    size_t m_offset{ 0 };
};

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
        // Output format: @rt_memcpy
        return std::format("@rt_{}", m_symbolName);
    }

  private:
    std::pmr::string m_symbolName;
};

class MirRegister : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::Register;

    MirRegister(MirType *type, bool isVirtual, size_t id, SourceReference *ref, std::pmr::string name = "") :
        MirOperand(type, ref), m_ref(RegisterRef::fromType(type, id, isVirtual)), m_name(std::move(name))
    {
    }

    bool isVirtual() const { return m_ref.isVirtual(); }
    bool operator==(const MirRegister &other) const { return m_ref == other.m_ref; }
    MirOperandType getType() const override { return OpKind; }
    RegisterRef getRef() const { return m_ref; }
    size_t getRegId() const { return m_ref.getId(); }
    void setRef(RegisterRef ref) { m_ref = ref; }

    std::string toString() const override
    {
        char prefix = m_ref.isVirtual() ? 'v' : 'p';
        if (!m_name.empty())
        {
            return std::format("{} %{}{}({})", getMirType()->getName(), prefix, m_ref.getId(), m_name);
        }
        return std::format("{} %{}{}", getMirType()->getName(), prefix, m_ref.getId());
    }

    const std::pmr::string &getName() const { return m_name; }

  private:
    RegisterRef m_ref;
    std::pmr::string m_name;
};

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
    MirOperandType getType() const override { return OpKind; }

    std::string toString() const override
    {
        std::string baseStr = m_base ? m_base->toString() : "0";

        if (m_displ && !m_displ->getValue().isZero())
        {
            // Output format: qword ptr [%v0 + 0x10]
            return std::format("ptr [{} + {}]", baseStr, m_displ->getValue().toString(16));
        }

        // Output format: qword ptr [%v0]
        return std::format("ptr [{}]", baseStr);
    }

  private:
    MirRegister *m_base;
    MirInteger *m_displ;
};

#endif // EZPACKER_MIROPERANDS_H