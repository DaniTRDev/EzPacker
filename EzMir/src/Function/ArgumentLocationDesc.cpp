#include "Function/ArgumentLocationDesc.h"
#include "Function/MirFunctionStackFrame.h"

/**
 * Stores the variant payload; callers should use the typed factory helpers instead.
 */
ArgumentLocationDesc::ArgumentLocationDesc(StorageT storage) : m_storage(std::move(storage)) {}

/**
 * Builds a register location describing which register holds the value and its byte size.
 */
ArgumentLocationDesc ArgumentLocationDesc::Reg(MirRegisterRef regId, size_t sizeInBytes)
{
    return ArgumentLocationDesc(RegLoc{ .m_ref = regId, .m_sizeBytes = sizeInBytes });
}
/**
 * Builds an indirect location, recording whether the pointee is passed by value, whether a copy
 * should be materialized in a register, the pointee size and the register holding the pointer.
 */
ArgumentLocationDesc ArgumentLocationDesc::Indirect(bool byVal, bool copyOnReg, size_t size, MirRegisterRef ptrStorage)
{
    return ArgumentLocationDesc(
            IndirectLoc{ .m_isByVal = byVal, .m_copyOnReg = copyOnReg, .m_size = size, .m_pointerStorage = ptrStorage });
}
/**
 * Builds a split location from the provided pieces, each describing how part of the argument is passed.
 */
ArgumentLocationDesc ArgumentLocationDesc::Split(std::vector<SplitPiece> pieces)
{
    return ArgumentLocationDesc(SplitLoc{ .m_parts = std::move(pieces) });
}

/**
 * Builds a stack location with its size and the stack frame object it refers to.
 */
ArgumentLocationDesc ArgumentLocationDesc::Stack(size_t sizeInBytes, StackFrameObject *object)
{
    return ArgumentLocationDesc(StackLoc{ .m_sizeBytes = sizeInBytes, .m_object = object });
}

/**
 * Returns the active location kind derived from the variant's active alternative.
 */
ArgLocationType ArgumentLocationDesc::getType() const
{
    switch (m_storage.index())
    {
        case 0:
            return ArgLocationType::Register;
        case 1:
            return ArgLocationType::Stack;
        case 2:
            return ArgLocationType::Split;
        case 3:
            return ArgLocationType::Indirect;
        default:
            return ArgLocationType::Invalid;
    }
}

/**
 * Returns the indirect payload, throwing a runtime_error if the active kind is not Indirect.
 */
const IndirectLoc &ArgumentLocationDesc::getIndirect() const
{
    if (!std::holds_alternative<IndirectLoc>(m_storage))
        throw std::runtime_error("ArgumentLocationDesc: Attempted to get Indirect from invalid variant state.");

    return std::get<IndirectLoc>(m_storage);
}

/**
 * Returns the register payload, throwing a runtime_error if the active kind is not Register.
 */
const RegLoc &ArgumentLocationDesc::getReg() const
{
    if (!std::holds_alternative<RegLoc>(m_storage))
        throw std::runtime_error("ArgumentLocationDesc: Attempted to get Reg from invalid variant state.");

    return std::get<RegLoc>(m_storage);
}

/**
 * Returns the split payload, throwing a runtime_error if the active kind is not Split.
 */
const SplitLoc &ArgumentLocationDesc::getSplit() const
{
    if (!std::holds_alternative<SplitLoc>(m_storage))
        throw std::runtime_error("ArgumentLocationDesc: Attempted to get Split from invalid variant state.");

    return std::get<SplitLoc>(m_storage);
}

/**
 * Returns the stack payload, throwing a runtime_error if the active kind is not Stack.
 */
const StackLoc &ArgumentLocationDesc::getStack() const
{
    if (!std::holds_alternative<StackLoc>(m_storage))
        throw std::runtime_error("ArgumentLocationDesc: Attempted to get Stack from invalid variant state.");

    return std::get<StackLoc>(m_storage);
}
