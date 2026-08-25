#ifndef EZMIR_ARGUMENT_LOCATION_DESC_H
#define EZMIR_ARGUMENT_LOCATION_DESC_H

#include "EzMirCommon.h"
#include "Operand/MirRegisterReference.h"

/**
 * Type alias representing a target physical hardware register ID.
 */
using PhysicalRegId = size_t;

/**
 * High-level ABI classification for parameter and return value placement.
 */
enum class ArgLocationType
{
    Invalid = 0, // Uninitialized or invalid location
    Indirect,    // Passed or returned indirectly by pointer (in register or stack slot)
    Register,    // Passed or returned directly in a single physical hardware register
    Split,       // Passed or returned split across multiple hardware registers (e.g. aggregate fields)
    Stack        // Passed or returned via caller/callee stack frame slot
};

/**
 * Direct physical register argument placement descriptor.
 */
struct RegLoc
{
    /**
     * Physical register reference.
     */
    MirRegisterRef m_ref;

    /**
     * Size of the passed value in bytes.
     */
    size_t m_sizeBytes;
};

/**
 * Stack frame argument placement descriptor.
 */
struct StackLoc
{
    /**
     * Size of the stack slot in bytes.
     */
    size_t m_sizeBytes;

    /**
     * Abstract stack frame object allocated for this argument.
     */
    class StackFrameObject *m_object;
};

/**
 * Segment descriptor for aggregate types passed or returned across multiple hardware registers.
 */
struct SplitPiece
{
    /**
     * Target physical register reference for this piece.
     */
    MirRegisterRef m_reg;

    /**
     * MirType of this component.
     */
    class MirType *m_type;

    /**
     * Byte offset from the beginning of the composite variable.
     */
    size_t m_offsetInParam;
};

/**
 * Multi-register placement descriptor for split aggregate arguments.
 */
struct SplitLoc
{
    /**
     * Ordered list of register chunks composing the split value.
     */
    std::vector<SplitPiece> m_parts;
};

/**
 * Indirect argument placement descriptor for pointer/by-value passing.
 */
struct IndirectLoc
{
    /**
     * True if callee receives a copy of the value (by-value semantics).
     */
    bool m_isByVal;

    /**
     * True if return pointer should be mirrored in standard return register (e.g. RAX).
     */
    bool m_copyOnReg;

    /**
     * Size of the underlying pointed-to object in bytes.
     */
    size_t m_size;

    /**
     * Physical register holding the pointer address.
     */
    MirRegisterRef m_pointerStorage;
};

/**
 * Discriminated union descriptor capturing the exact ABI-lowered location of a function argument or return value.
 */
class ArgumentLocationDesc
{
  public:
    using StorageT = std::variant<RegLoc, StackLoc, SplitLoc, IndirectLoc>;

    /**
     * Factory constructing a direct physical register location descriptor.
     */
    static ArgumentLocationDesc Reg(MirRegisterRef reg, size_t sizeInBytes);

    /**
     * Factory constructing an indirect pointer-passed location descriptor.
     */
    static ArgumentLocationDesc Indirect(bool byVal, bool copyOnReg, size_t size, MirRegisterRef ptrStorage);

    /**
     * Factory constructing a split multi-register location descriptor.
     */
    static ArgumentLocationDesc Split(const std::vector<SplitPiece> &pieces);

    /**
     * Factory constructing a stack frame slot location descriptor.
     */
    static ArgumentLocationDesc Stack(size_t sizeInBytes, class StackFrameObject *object);

    /**
     * Returns the active location category (Register, Indirect, Split, Stack).
     */
    ArgLocationType getType() const;

    /**
     * Retrieves the indirect location payload, throwing std::runtime_error if type mismatch.
     */
    const IndirectLoc &getIndirect() const;

    /**
     * Retrieves the direct register location payload, throwing std::runtime_error if type mismatch.
     */
    const RegLoc &getReg() const;

    /**
     * Retrieves the split multi-register location payload, throwing std::runtime_error if type mismatch.
     */
    const SplitLoc &getSplit() const;

    /**
     * Retrieves the stack slot location payload, throwing std::runtime_error if type mismatch.
     */
    const StackLoc &getStack() const;

  private:
    /**
     * Private constructor initializing active type tag and variant storage payload.
     */
    ArgumentLocationDesc(ArgLocationType type, StorageT storage);

  private:
    /**
     * Active discriminator tag.
     */
    ArgLocationType m_type;

    /**
     * Variant storage container.
     */
    StorageT m_storage;
};

#endif // EZMIR_ARGUMENT_LOCATION_DESC_H
