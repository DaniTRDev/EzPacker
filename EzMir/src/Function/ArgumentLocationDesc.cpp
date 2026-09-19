#include "Function/ArgumentLocationDesc.h"
#include "Function/MirFunctionStackFrame.h"

/**
 * Stores the location kind and its variant payload; callers should use the typed factory helpers instead.
 */
ArgumentLocationDesc::ArgumentLocationDesc(ArgLocationType type, StorageT storage) : m_type(type), m_storage(storage) {}

/**
 * Builds a register location describing which register holds the value and its byte size.
 */
ArgumentLocationDesc ArgumentLocationDesc::Reg(MirRegisterRef regId, size_t sizeInBytes)
{
    return ArgumentLocationDesc(ArgLocationType::Register, RegLoc{ .m_ref = regId, .m_sizeBytes = sizeInBytes });
}
/**
 * Builds an indirect location, recording whether the pointee is passed by value, whether a copy
 * should be materialized in a register, the pointee size and the register holding the pointer.
 */
ArgumentLocationDesc ArgumentLocationDesc::Indirect(bool byVal, bool copyOnReg, size_t size, MirRegisterRef ptrStorage)
{
    return ArgumentLocationDesc(ArgLocationType::Indirect,
                                IndirectLoc{ .m_isByVal = byVal,
                                             .m_copyOnReg = copyOnReg,
                                             .m_size = size,
                                             .m_pointerStorage = ptrStorage });
}
/**
 * Builds a split location from the provided pieces, each describing how part of the argument is passed.
 */
ArgumentLocationDesc ArgumentLocationDesc::Split(const std::vector<SplitPiece> &pieces)
{
    return ArgumentLocationDesc(ArgLocationType::Split, SplitLoc{ .m_parts = std::move(pieces) });
}

/**
 * Builds a stack location with its size and the stack frame object it refers to.
 */
ArgumentLocationDesc ArgumentLocationDesc::Stack(size_t sizeInBytes, StackFrameObject *object)
{
    return ArgumentLocationDesc(ArgLocationType::Stack, StackLoc{ .m_sizeBytes = sizeInBytes, .m_object = object });
}

/**
 * Returns the active location kind, indicating which getter is valid.
 */
ArgLocationType ArgumentLocationDesc::getType() const { return m_type; }

/**
 * Returns the indirect payload, throwing a runtime_error if the active kind is not Indirect.
 */
const IndirectLoc &ArgumentLocationDesc::getIndirect() const
{
    if (m_type != ArgLocationType::Indirect)
        throw std::runtime_error("ArgumentLocationDesc: Attempted to get Indirect from invalid variant state.");

    return std::get<IndirectLoc>(m_storage);
}

/**
 * Returns the register payload, throwing a runtime_error if the active kind is not Register.
 */
const RegLoc &ArgumentLocationDesc::getReg() const
{
    if (m_type != ArgLocationType::Register)
        throw std::runtime_error("ArgumentLocationDesc: Attempted to get Reg from invalid variant state.");

    return std::get<RegLoc>(m_storage);
}

/**
 * Returns the split payload, throwing a runtime_error if the active kind is not Split.
 */
const SplitLoc &ArgumentLocationDesc::getSplit() const
{
    if (m_type != ArgLocationType::Split)
        throw std::runtime_error("ArgumentLocationDesc: Attempted to get Split from invalid variant state.");

    return std::get<SplitLoc>(m_storage);
}

/**
 * Returns the stack payload, throwing a runtime_error if the active kind is not Stack.
 */
const StackLoc &ArgumentLocationDesc::getStack() const
{
    if (m_type != ArgLocationType::Stack)
        throw std::runtime_error("ArgumentLocationDesc: Attempted to get Stack from invalid variant state.");

    return std::get<StackLoc>(m_storage);
}
