#ifndef EZMIR_CLASS_OFFSET_RESOLVER_PASS_H
#define EZMIR_CLASS_OFFSET_RESOLVER_PASS_H

#include "EzMirCommon.h"
#include "MirPasses/IMirTransformPass.h"

/**
 * This pass iterates over the classes defined in a context and resolves the offsets of both the methods and fields of a
 * class.
 *
 * This pass doesn't care if a class was already resolved, this is done this way just in case this pass is run a second
 * time after another pass modified fields/methods after running this pass in the first time.
 */
class ClassOffsetResolverPass : public IMirTransformPass
{
  public:
    /**
     * Creates the pass with the given context.
     */
    ClassOffsetResolverPass(class MirBuilderContext *ctx);

    /**
     * Returns the name of the pass "ClassOffsetResolvesPass".
     */
    const char *getName() const override;

    /**
     * Returns MirPassIterationPlace::Class.
     */
    MirPassIterationPlace getIterationPlace() const override;

    /**
     * Runs the pass in the given class and returns the result.
     */
    MirPassResult run(class MirClass *_class, class MirPassManager *passManager) override;

    /**
     * Prints the modified class list which will have their offsets resolved.
     */
    void printResult() override;

  private:
    class MirBuilderContext *m_ctx;
    std::map<MirId, class MirClass *> m_resolvedClasses;
};

#endif // EZMIR_CLASS_OFFSET_RESOLVER_PASS_H