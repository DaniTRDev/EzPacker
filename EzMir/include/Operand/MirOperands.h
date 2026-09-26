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

/**
 * Formats an operand for MIR text.
 *
 * In parseable mode this produces strict, round-trippable .mir syntax (a single shared
 * implementation consumed by the printer). In non-parseable mode it returns the operand's own
 * detailed diagnostic rendering.
 */
std::string formatOperand(MirOperand *operand, bool parseable);

/**
 * Classification of symbolic targets referenced by MirReference operands.
 */
enum class MirReferenceType : uint8_t
{
    Invalid = 0,     // Invalid or unassigned reference type sentinel
    Block,           // Reference to a MirBlock label (branch / jump target)
    Function,        // Reference to a MirFunction entry point (call target)
    GlobalVar,       // Reference to a MirGlobalVar memory location
    StackFrameObject // Reference to a local stack slot inside the function's MirFunctionStackFrame
};

/**
 * Concrete operand representing an IEEE-754 constant floating-point value (FlexFloat).
 */
class MirFloat : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::FloatingPoint;

    /**
     * Constructs a floating-point literal operand.
     */
    MirFloat(MirType *type, FlexFloat value, SourceReference *ref);

    /**
     * Returns mutable reference to the stored floating-point value.
     */
    FlexFloat &getValue() { return m_float; }

    /**
     * Returns const reference to the stored floating-point value.
     */
    const FlexFloat &getValue() const { return m_float; }

    /**
     * Returns MirOperandType::FloatingPoint.
     */
    MirOperandType getType() const override { return OpKind; }

    /**
     * Formats the float operand as type and decimal representation.
     */
    std::string toString() const override;

  private:
    /**
     * Multi-precision floating-point number payload.
     */
    FlexFloat m_float;
};

/**
 * Concrete operand representing a constant integer value (FlexInt).
 */
class MirInteger : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::Integer;

    /**
     * Constructs an integer literal operand.
     */
    MirInteger(MirType *type, FlexInt value, SourceReference *ref);

    /**
     * Returns mutable reference to the stored integer value.
     */
    FlexInt &getValue() { return m_int; }

    /**
     * Returns const reference to the stored integer value.
     */
    const FlexInt &getValue() const { return m_int; }

    /**
     * Returns MirOperandType::Integer.
     */
    MirOperandType getType() const override { return OpKind; }

    /**
     * Formats the integer operand as type and decimal or hex representation.
     */
    std::string toString() const override;

  private:
    /**
     * Multi-precision integer number payload.
     */
    FlexInt m_int;
};

/**
 * Concrete operand representing a symbolic address or reference to an IR entity (block, function, global, stack slot).
 */
class MirReference : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::Reference;

    /**
     * Constructs a symbolic reference operand with target kind, entity ID, byte offset, and source ref.
     */
    MirReference(MirType *type, MirReferenceType refType, size_t refId, size_t offset, SourceReference *ref);

    /**
     * Returns true if referencing a basic block.
     */
    bool isBlock() const { return m_refType == MirReferenceType::Block; }

    /**
     * Returns true if referencing a global variable.
     */
    bool isGlobalVar() const { return m_refType == MirReferenceType::GlobalVar; }

    /**
     * Returns true if referencing a function.
     */
    bool isFunction() const { return m_refType == MirReferenceType::Function; }

    /**
     * Returns true if referencing a stack frame object slot.
     */
    bool isStackFrameObject() const { return m_refType == MirReferenceType::StackFrameObject; }

    /**
     * Returns true if this reference is unassigned or invalid.
     */
    bool isInvalid() const { return m_refType == MirReferenceType::Invalid; }

    /**
     * Returns the target reference kind.
     */
    MirReferenceType getRefType() const { return m_refType; }

    /**
     * Returns MirOperandType::Reference.
     */
    MirOperandType getType() const override { return OpKind; }

    /**
     * Returns the unique ID of the referenced entity.
     */
    size_t getRefId() const { return m_refId; }

    /**
     * Returns the byte displacement/offset from the entity base address.
     */
    size_t getOffset() const { return m_offset; }

    /**
     * Formats the symbolic reference into a string representation.
     */
    std::string toString() const override;

  private:
    /**
     * Kind of entity referenced.
     */
    MirReferenceType m_refType;

    /**
     * Identifier of referenced block, function, global, or stack slot.
     */
    size_t m_refId{ 0 };

    /**
     * Byte offset relative to the base symbol address.
     */
    size_t m_offset{ 0 };
};

/**
 * Concrete operand representing a named external symbol from the runtime support library.
 */
class MirRuntimeSymbol : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::RuntimeSymbol;

    /**
     * Constructs a runtime symbol operand with the given name.
     */
    MirRuntimeSymbol(MirType *type, std::pmr::string name, SourceReference *ref);

    /**
     * Returns MirOperandType::RuntimeSymbol.
     */
    MirOperandType getType() const override { return OpKind; }

    /**
     * Returns the runtime symbol name.
     */
    const std::pmr::string &getSymbolName() const { return m_symbolName; }

    /**
     * Formats the runtime symbol as @name.
     */
    std::string toString() const override;

  private:
    /**
     * Symbolic identifier name in the runtime library.
     */
    std::pmr::string m_symbolName;
};

/**
 * Concrete operand representing a virtual or physical register.
 */
class MirRegister : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::Register;

    /**
     * Constructs a register operand.
     */
    MirRegister(MirType *type,
                bool isVirtual,
                size_t id,
                SourceReference *ref,
                MirRegisterClass *_class = nullptr,
                std::pmr::string name = "");

    /**
     * Returns true if this is a virtual SSA / pre-allocation register.
     */
    bool isVirtual() const { return m_ref.isVirtual(); }

    /**
     * Returns true if this is a physical hardware register.
     */
    bool isPhysical() const { return !m_ref.isVirtual(); }

    /**
     * Equality operator based on underlying register reference.
     */
    bool operator==(const MirRegister &other) const { return m_ref == other.m_ref; }

    /**
     * Returns MirOperandType::Register.
     */
    MirOperandType getType() const override { return OpKind; }

    /**
     * Returns the register class constraint descriptor.
     */
    MirRegisterClass *getRegClass() { return m_ref.getClass(); }

    /**
     * Returns the underlying MirRegisterRef structure.
     */
    MirRegisterRef getRef() const { return m_ref; }

    /**
     * Returns the numeric ID of the register.
     */
    size_t getRegId() const { return m_ref.getId(); }

    /**
     * Assigns or updates the physical register class constraint.
     */
    void setClass(MirRegisterClass *_class) { m_ref.setClass(_class); }

    /**
     * Updates the underlying register reference.
     */
    void setRef(MirRegisterRef ref) { m_ref = ref; }

    /**
     * Returns the optional debug name assigned to the register.
     */
    const std::pmr::string &getName() const { return m_name; }

    /**
     * Formats the register into diagnostic string form (%v1, %p0(rax:GPR64), etc.).
     */
    std::string toString() const override;

  private:
    /**
     * Internal register reference descriptor.
     */
    MirRegisterRef m_ref;

    /**
     * Informational diagnostic name.
     */
    std::pmr::string m_name;
};

/**
 * Concrete operand representing a base-plus-displacement memory addressing mode ([base + displacement]).
 */
class MirMemory : public MirOperand
{
  public:
    static constexpr MirOperandType OpKind = MirOperandType::Memory;

    /**
     * Constructs a memory operand with base pointer register, displacement, optional index register, and scale.
     */
    MirMemory(MirType *type,
              MirRegister *base,
              MirInteger *displ,
              MirRegister *index = nullptr,
              uint8_t scale = 1,
              SourceReference *ref = nullptr);

    /**
     * Returns pointer register holding the base address.
     */
    MirRegister *getBase() const { return m_base; }

    /**
     * Returns integer operand holding the byte displacement.
     */
    MirInteger *getDisplacement() const { return m_displ; }

    /**
     * Returns index register holding scaled offset, or nullptr.
     */
    MirRegister *getIndex() const { return m_index; }

    /**
     * Returns scale multiplier for index register (1, 2, 4, 8).
     */
    uint8_t getScale() const { return m_scale; }

    /**
     * Returns true if a valid base register is set.
     */
    bool hasBaseReg() const { return m_base != nullptr; }

    /**
     * Returns true if a valid index register is set.
     */
    bool hasIndexReg() const { return m_index != nullptr; }

    /**
     * Sets the base pointer register.
     */
    void setBase(MirRegister *base) { m_base = base; }

    /**
     * Sets the index register.
     */
    void setIndex(MirRegister *index) { m_index = index; }

    /**
     * Returns true if this is simple [base + displacement] with no index.
     */
    bool isSimpleBaseDisp() const { return m_index == nullptr && m_scale <= 1; }

    /**
     * Returns MirOperandType::Memory.
     */
    MirOperandType getType() const override { return OpKind; }

    /**
     * Formats memory operand as ptr [base + index*scale + offset].
     */
    std::string toString() const override;

  private:
    /**
     * Base register pointer.
     */
    MirRegister *m_base{ nullptr };

    /**
     * Displacement offset operand.
     */
    MirInteger *m_displ{ nullptr };

    /**
     * Optional index register pointer.
     */
    MirRegister *m_index{ nullptr };

    /**
     * Scale multiplier for index register (typically 1, 2, 4, 8).
     */
    uint8_t m_scale{ 1 };
};

#endif // EZMIR_MIR_OPERANDS_H