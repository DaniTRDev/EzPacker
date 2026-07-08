#ifndef EZPACKER_MIROPERANDS_H
#define EZPACKER_MIROPERANDS_H

#include "EzMirCommon.h"
#include "MirOperand.h"

/**
 * Constructors are made private to ensure that operands are created using MirOperandBuilder.
 */

enum class MirReferenceType : uint8_t
{
    Invalid = 0,
    Block,
    DataEntry,
    Function
};

class MirFloat : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::FloatingPoint;

    MirFloat(MirType *type, std::pmr::string value, SourceReference *ref) :
        MirOperand(type, ref), m_value(std::move(value))
    {
    }

    const std::pmr::string &getValue() const { return m_value; }
    MirOperandType getType() const override { return OpKind; }
    std::string toString() const override
    {
        return std::format("{} %float.value={}", getMirType()->getName(), m_value);
    }

  private:
    std::pmr::string m_value; /*
                               * Immediate floating-point literal that can have any given precision (dictated by
                               * underlying MirType)
                               */
};

class MirInteger : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::Integer;

    MirInteger(MirType *type, int64_t value, SourceReference *ref) : MirOperand(type, ref), m_value(value) {}

    int64_t getValue() const { return m_value; }
    MirOperandType getType() const override { return OpKind; }
    std::string toString() const override { return std::format("{} %int.value={}", getMirType()->getName(), m_value); }

  private:
    int64_t m_value{ 0 };
};

class MirReference : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::Reference;

    MirReference(MirType *type, MirReferenceType refType, size_t refId, SourceReference *ref) :
        MirOperand(type, ref), m_refType(refType), m_refId(refId)
    {
    }

    bool isBlock() const { return m_refType == MirReferenceType::Block; }
    bool isDataEntry() const { return m_refType == MirReferenceType::DataEntry; }
    bool isFunction() const { return m_refType == MirReferenceType::Function; }
    bool isInvalid() const { return m_refType == MirReferenceType::Invalid; }

    MirReferenceType getRefType() const { return m_refType; }
    MirOperandType getType() const override { return OpKind; }
    size_t getRefId() const { return m_refId; }

    std::string toString() const override
    {
        std::string src = "INVALID";
        if (isBlock())
        {
            src = "block";
        }
        else if (isDataEntry())
        {
            src = "data";
        }
        else if (isFunction())
        {
            src = "func";
        }

        return std::format("{} %ref.id={}.src={}", getMirType()->getName(), m_refId, src);
    }

  private:
    MirReferenceType m_refType;
    size_t m_refId{ 0 }; // Generic MIR reference ID (block, function, data entry, ...).
};

class MirRegister : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::Register;

    MirRegister(MirType *type, bool isVirtual, size_t id, SourceReference *ref, std::pmr::string name = "") :
        MirOperand(type, ref), m_virtual(isVirtual), m_id(id), m_name(std::move(name))
    {
    }

    bool isVirtual() const { return m_virtual; }
    size_t getRegId() const { return m_id; }
    bool operator==(const MirRegister &other) const { return m_id == other.m_id && m_virtual == other.m_virtual; }

    const std::pmr::string &getName() const { return m_name; }

    MirOperandType getType() const override { return OpKind; }
    std::string toString() const override
    {
        if (!m_name.empty())
        {
            return std::format("{} {}.name={}", getMirType()->getName(), isVirtual() ? "%v" : "%p", m_name);
        }

        return std::format("{} {}.id={}", getMirType()->getName(), isVirtual() ? "%v" : "%p", m_id);
    }

    void setRegId(size_t id) { m_id = id; }
    void setVirtual(bool value) { m_virtual = value; }

  private:
    bool m_virtual{ true }; // Whether this is a virtual register (true) or a physical register (false).
    size_t m_id{ 0 };       // Unique register ID.
    std::pmr::string m_name;
};

class MirFrameIndex : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::FrameIndex;

    MirFrameIndex(MirType *type, size_t frameId, SourceReference *ref) : MirOperand(type, ref), m_frameId(frameId) {}

    size_t getFrameId() const { return m_frameId; }
    MirOperandType getType() const override { return OpKind; }
    std::string toString() const override { return std::format("{} %frame.id={}", getMirType()->getName(), m_frameId); }

  private:
    // Used to reference parameters and objects that are saved in a stack frame.
    size_t m_frameId{ 0 };
};

class MirMemory : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::Memory;

    // The 'type' passed to the base constructor represents the size of what's being accessed.
    MirMemory(MirType *type, MirOperand *base, MirOperand *displ, SourceReference *ref) :
        MirOperand(type, ref), m_base(base), m_displ(displ)
    {
    }

    MirOperand *getBase() const { return m_base; }
    MirOperand *getDisplacement() const { return m_displ; }

    MirOperandType getType() const override { return OpKind; }
    std::string toString() const override
    {
        return std::format("{} %mem.base={}.index={}",
                           getMirType()->getName(),
                           m_base ? m_base->toString() : "",
                           m_displ ? m_displ->toString() : "");
    }

  private:
    MirOperand *m_base;
    MirOperand *m_displ;
};

#endif // EZPACKER_MIROPERANDS_H