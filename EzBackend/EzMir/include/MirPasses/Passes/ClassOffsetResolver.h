#ifndef EZPACKER_CLASSOFFSETRESOLVER_H
#define EZPACKER_CLASSOFFSETRESOLVER_H

#include "EzMirCommon.h"
#include "Builder/MirBuilderContext.h"
#include "MirPasses/IMirTransformPass.h"
#include "Printer/MirPrinter.h"

/**
 * This pass iterates over the classes defined in a context and resolves the offsets of both the methods and fields of a
 * class.
 *
 * This pass doesn't care if a class was already resolved, this is done this way just in case this pass is run a second
 * time after another pass modified fields/methods after running this pass in the first time.
 */
class ClassOffsetResolver : public IMirTransformPass
{
  public:
    /**
     * Creates the pass with the given context.
     */
    ClassOffsetResolver(MirBuilderContext *ctx);

    /**
     * Returns the name of the pass "ClassOffsetResolvesPass".
     * @return
     */
    const char *getName() const override;

    /**
     * Returns MirPassIterationPlace::Class.
     * @return
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Runs the pass in the given class and returns the result.
     * @param instrList
     * @param it
     * @param passManager
     * @return
     */
    MirPassResult run(MirClass *_class, MirPassManager *passManager) override;

    /**
     * Prints the modified class list which will have their offsets resolved.
     */
    void printResult() const override;

  private:
    MirBuilderContext *m_ctx;
    std::map<MirId, MirClass *> m_resolvedClasses;
};

#endif // EZPACKER_CLASSOFFSETRESOLVER_H