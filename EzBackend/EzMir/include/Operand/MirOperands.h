#ifndef EZPACKER_MIROPERANDS_H
#define EZPACKER_MIROPERANDS_H

#include "EzMirCommon.h"
#include "MirOperand.h"

enum class MirReferenceType : uint8_t
{
    Invalid = 0,
    Block,
    DataEntry,
    Function
};

class MirDouble : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::Double;

    MirDouble(MirType *type, double value, SourceReference *ref) : MirOperand(type, ref), m_value(value) {}

    double getValue() const { return m_value; }
    MirOperandType getType() const override { return OpKind; }
    std::string toString() const override { return std::format("double({})", std::to_string(m_value)); }

  private:
    double m_value{ 0.0 }; // Immediate floating-point literal.
};

class MirInteger : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::Integer;

    MirInteger(MirType *type, int64_t value, SourceReference *ref) : MirOperand(type, ref), m_value(value) {}

    int64_t getValue() const { return m_value; }
    MirOperandType getType() const override { return OpKind; }
    std::string toString() const override { return std::format("integer({})", std::to_string(m_value)); }

  private:
    int64_t m_value{ 0 }; // Immediate signed integer literal.
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
        return std::format("@ref({}) (type: {})", m_refId, getMirType()->getName());
    }

  private:
    MirReferenceType m_refType;
    size_t m_refId{ 0 }; // Generic MIR reference ID (block, function, data entry, ...).
};

class MirRegister : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::Register;

    MirRegister(MirType *type, bool isVirtual, size_t id, SourceReference *ref, const char *name = nullptr) :
        MirOperand(type, ref), m_virtual(isVirtual), m_id(id), m_name(name)
    {
    }

    bool isVirtual() const { return m_virtual; }
    size_t getRegId() const { return m_id; }
    bool operator==(const MirRegister &other) const { return m_id == other.m_id && m_virtual == other.m_virtual; }

    const char *getName() const { return m_name; }

    MirOperandType getType() const override { return OpKind; }
    std::string toString() const override
    {
        return std::format("@reg (id: {}) (name: {})", m_id, m_name ? m_name : "NO_NAME");
    }

    void setRegId(size_t id) { m_id = id; }
    void setVirtual(bool value) { m_virtual = value; }

  private:
    bool m_virtual{ true }; // Whether this is a virtual register (true) or a physical register (false).
    const char *m_name;
    size_t m_id{ 0 }; // Unique register ID.
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
        return std::format("@frame({}) (type: {})", m_frameId, getMirType()->getName());
    }

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
        return std::format("@mem({},{})",
                           m_base ? m_base->toString() : "NO_BASE",
                           m_displ ? m_displ->toString() : "NO_DISPL");
    }

  private:
    MirOperand *m_base;
    MirOperand *m_displ;
};

#endif // EZPACKER_MIROPERANDS_H