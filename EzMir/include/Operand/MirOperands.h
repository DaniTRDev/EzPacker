#ifndef EZMIR_MIR_OPERANDS_H
#define EZMIR_MIR_OPERANDS_H

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
    Block,           // A reference to a code block.
    ClassField,      // A reference to the field of a class.
    ClassMethod,     // A reference to a method of a class.
    Function,        // A reference to a function.
    GlobalVar,       // A reference to a global variable.
    StackFrameObject // A reference to a stack frame object.
};

/**
 * Represents a constant compile-time floating point value. It follows IEEE-764.
 */
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

/**
 * Represents a constant compile-time integer value.
 */
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

/**
 *This represents a pointer to a symbol. This can be fed to memory operands in STORE/LOAD instructions to dereference
 *the address.
 */
class MirReference : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::Reference;

    MirReference(MirType *type, MirReferenceType refType, size_t refId, size_t offset, SourceReference *ref);

    bool isBlock() const { return m_refType == MirReferenceType::Block; }
    bool isGlobalVar() const { return m_refType == MirReferenceType::GlobalVar; }
    bool isFunction() const { return m_refType == MirReferenceType::Function; }
    bool isClassField() const { return m_refType == MirReferenceType::ClassField; }
    bool isClassMethod() const { return m_refType == MirReferenceType::ClassMethod; }
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
    MirRegisterClass *getRegClass() { return m_ref.getClass(); }
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

#endif // EZMIR_MIR_OPERANDS_H