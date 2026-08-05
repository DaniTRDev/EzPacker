#ifndef EZTRIPLETESTFRAMELOWERER_H
#define EZTRIPLETESTFRAMELOWERER_H

#include "EzTripleTestSuite.h"

class EzTripleTestFrameLowerer : public MirFrameLowerer
{
  public:
    /**
     * Inserts the prologue of the function. calculateFrameLayout must have been called before.
     */
    void insertPrologue(FrameLowererCtx &ctx) override;

    /**
     * Inserts the epilogue of the function. calculateFrameLayout must have been called before.
     */
    void insertEpilogue(FrameLowererCtx &ctx) override;
};

#endif // EZTRIPLETESTFRAMELOWERER_H
