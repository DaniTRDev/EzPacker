#ifndef EZPACKER_EZTRIPLETESTSELECTOR_H
#define EZPACKER_EZTRIPLETESTSELECTOR_H

#include "EzTriple.h"
#include "../../EzMirTestSuite/include/EzMirTestSuite.h"
#include "EzTripleTestInstructionSet.h"

class EzTripleTestSelector
{
  public:
    /**
     * Appends the selection rules in a given created selector.
     */
    static void create(MirBuilderContext *ctx, MirInstructionSelector *selector);

  private:
};

#endif // EZPACKER_EZTRIPLETESTSELECTOR_H
