#ifndef EZPACKER_TYPELOWERER_H
#define EZPACKER_TYPELOWERER_H

#include "EzSemanticsCommon.h"
#include "GenericAstLowerer.h"

class TypeLowerer
{
  public:
    /**
     * Tries to lower the types generated in the semantic phase and convert them to MIR.
     * Returns `true` if the type could be lowered. If type already exists, `false` is returned.
     */
    bool lower(TypeTable *table, LoweringContext *ctx);
};

#endif // EZPACKER_TYPELOWERER_H
