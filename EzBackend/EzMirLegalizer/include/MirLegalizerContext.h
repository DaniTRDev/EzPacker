#ifndef EZPACKER_MIRLEGALIZERCONTEXT_H
#define EZPACKER_MIRLEGALIZERCONTEXT_H

#include "EzMirLegalizerCommon.h"

class MirLegalizerContext
{
  public:
    /**
     * Sets the emitter of the context.
     *
     * @param emitter
     */
    void setEmitter(MirEmitter *emitter);

    /**
     * Sets the ABIDesc of the context.
     * @param abiDesc
     */
    void setAbiDesc(ABIDesc *abiDesc);

    /**
     * Sets the pass manager of the context.
     * @param passManager
     */
    void setPassManager(MirPassManager *passManager);

    /**
     * Returns the emitter.
     * @return
     */
    MirEmitter *getEmitter() const;

    /**
     * Returns the ABIDesc.
     * @return
     */
    ABIDesc *getAbiDesc() const;

    /**
     * Returns the pass manager.
     * @return
     */
    MirPassManager *getPassManager() const;

  private:
    MirEmitter *m_emitter;
    ABIDesc *m_abiDesc;
    MirPassManager *m_passManager;
};

#endif // EZPACKER_MIRLEGALIZERCONTEXT_H
