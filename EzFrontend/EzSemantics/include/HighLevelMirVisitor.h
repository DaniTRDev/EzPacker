#ifndef EZPACKER_HIGHLEVELMIRVISITOR_H
#define EZPACKER_HIGHLEVELMIRVISITOR_H

#include "EzSemanticsCommon.h"
#include "HighLevelMir/HighLevelMirDefs.h"

/**
 * Class used to traverse the flattened HLMIR instruction list.
 */
class HighLevelMirVisitor
{
  public:
    virtual ~HighLevelMirVisitor() = default;

    /**
     * Visits the given MIR module.
     * @param module
     * @return bool
     */
    virtual bool visit(const class HighLevelMirModule &module) { return true; }

    /**
     * Visits the given MIR block.
     * @param block
     * @return bool
     */
    virtual bool visit(const class HighLevelMirBlock &block) { return true; }

    /**
     * Visits the given MIR instruction.
     * @param instruction
     * @return bool
     */
    virtual bool visit(const class HighLevelMirInstruction &instruction) { return true; }

    /**
     * Sets the semantic context.
     * @param ctx
     */
    void setSemanticContext(const std::shared_ptr<class BasicSemanticContext> &ctx);

    /**
     * Returns the semantic context of this object.
     * @return const std::shared_ptr<BasicSemanticContext> &
     */
    const std::shared_ptr<class BasicSemanticContext> &getSemanticContext() const;

  protected:
    std::shared_ptr<class BasicSemanticContext> m_ctx;
};
#endif // EZPACKER_HIGHLEVELMIRVISITOR_H
