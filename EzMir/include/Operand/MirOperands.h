#ifndef EZPMIR_MIR_OPERANDS_H
#define EZPMIR_MIR_OPERANDS_H

#include "FlexNumber/FlexFloat.h"
#include "FlexNumber/FlexInt.h"

#include "MirOperand.h"
#include "MirRegisterReference.h"

// Forward declarations
class MirType;
class SourceReference;
class MirRegisterClass;

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

    MirFloat(MirType *type, FlexFloat value, SourceReference *ref);

    FlexFloat &getValue() { return m_float; }
    const FlexFloat &getValue() const { return m_float; }
    MirOperandType getType() const override { return OpKind; }

    std::string toString() const override;

  private:
    FlexFloat m_float;
};

class MirInteger : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::Integer;

    MirInteger(MirType *type, FlexInt value, SourceReference *ref);

    FlexInt &getValue() { return m_int; }
    const FlexInt &getValue() const { return m_int; }
    MirOperandType getType() const override { return OpKind; }

    std::string toString() const override;

  private:
    FlexInt m_int;
};

class MirConstantArray : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::ConstantArray;

    MirConstantArray(MirType *arrayType, std::pmr::vector<MirOperand *> elements, SourceReference *ref);

    const std::pmr::vector<MirOperand *> &getElements() const { return m_elements; }
    MirOperandType getType() const override { return OpKind; }

    std::string toString() const override;

  private:
    std::pmr::vector<MirOperand *> m_elements;
};

class MirReference : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::Reference;

    MirReference(MirType *type, MirReferenceType refType, size_t refId, size_t offset, SourceReference *ref);

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

    std::string toString() const override;

  private:
    MirReferenceType m_refType;
    size_t m_refId{ 0 };
    size_t m_offset{ 0 };
};

class MirRuntimeSymbol : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::RuntimeSymbol;

    MirRuntimeSymbol(MirType *type, std::pmr::string name, SourceReference *ref);

    MirOperandType getType() const override { return OpKind; }
    const std::pmr::string &getSymbolName() const { return m_symbolName; }

    std::string toString() const override;

  private:
    std::pmr::string m_symbolName;
};

class MirRegister : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::Register;

    MirRegister(MirType *type,
                bool isVirtual,
                size_t id,
                SourceReference *ref,
                MirRegisterClass *_class = nullptr,
                std::pmr::string name = "");

    bool isVirtual() const { return m_ref.isVirtual(); }
    bool isPhysical() const { return !m_ref.isVirtual(); }
    bool operator==(const MirRegister &other) const { return m_ref == other.m_ref; }

    MirOperandType getType() const override { return OpKind; }
    MirRegisterRef getRef() const { return m_ref; }
    size_t getRegId() const { return m_ref.getId(); }

    void setClass(MirRegisterClass *_class) { m_ref.setClass(_class); }
    void setRef(MirRegisterRef ref) { m_ref = ref; }

    const std::pmr::string &getName() const { return m_name; }

    std::string toString() const override;

  private:
    MirRegisterRef m_ref;
    std::pmr::string m_name;
};

class MirFrameIndex : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::FrameIndex;

    MirFrameIndex(MirType *type, size_t frameId, SourceReference *ref);

    size_t getFrameId() const { return m_frameId; }
    MirOperandType getType() const override { return OpKind; }

    std::string toString() const override;

  private:
    size_t m_frameId{ 0 };
};

class MirMemory : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::Memory;

    MirMemory(MirType *type, MirRegister *base, MirInteger *displ, SourceReference *ref);

    MirRegister *getBase() const { return m_base; }
    MirInteger *getDisplacement() const { return m_displ; }
    bool hasBaseReg() const { return m_base != nullptr; }
    MirOperandType getType() const override { return OpKind; }

    std::string toString() const override;

  private:
    MirRegister *m_base;
    MirInteger *m_displ;
};

#endif // EZPMIR_MIR_OPERANDS_H