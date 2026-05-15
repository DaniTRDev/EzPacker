#ifndef EZPACKER_EXPANDTYPELEGALIZER_H
#define EZPACKER_EXPANDTYPELEGALIZER_H

#include "EzTargetCommon.h"
#include "TargetLegalizer/LegalizerContext.h"

namespace StandardLegalizers
{
/**
 * Legalizes an instruction by expanding its illegal operandIt and result types to the next legal type. If a given
 * illegal type of size N is given, it will return 2 N/2 hi-low sub types. This function also takes in account
 * whether if the input instr is an add (when expanded must be set to add + adc), sub (sub + sbb), ...
 * @param emitter
 * @param targetDesc
 * @param instrList
 * @param it
 */
extern bool expandTypeLegalizer(LegalizerContext *ctx,
                                TypedPoolLinkedList<class MirInstruction> *instrList,
                                TypedPoolLinkedList<class MirInstruction>::Iterator it);
}; // namespace StandardLegalizers

#endif // EZPACKER_EXPANDTYPELEGALIZER_H
