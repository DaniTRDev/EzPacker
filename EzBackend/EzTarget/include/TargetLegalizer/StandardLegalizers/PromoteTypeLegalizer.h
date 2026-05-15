#ifndef EZPACKER_PROMOTETYPELEGALIZER_H
#define EZPACKER_PROMOTETYPELEGALIZER_H

#include "EzTargetCommon.h"

namespace StandardLegalizers
{
/**
 * Legalizes an instruction by promoting its illegal operand and result types to the next legal type. This is a
 * common legalization strategy for integer types, where smaller integers are promoted to a larger integer type that
 * is natively supported by the target architecture.
 *
 * Returns true if the instr / instrList was modified.
 * @param emitter
 * @param targetDesc
 * @param instrList
 * @param it
 */
extern bool promoteTypeLegalizer(MirEmitter *emitter,
                                 class TargetDesc *targetDesc,
                                 TypedPoolLinkedList<class MirInstruction>::Iterator it,
                                 TypedPoolLinkedList<class MirOperand>::Iterator operand,
                                 size_t operandId);
}; // namespace StandardLegalizers

#endif // EZPACKER_PROMOTETYPELEGALIZER_H
