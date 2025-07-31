#ifndef EZPACKER_MODULEANNOTATOR_H
#define EZPACKER_MODULEANNOTATOR_H

#include "EzAnnotatorCommon.h"
#include "ScopeAbleAnnotator.h"
#include "InstructionAnnotator.h"
#include "LabelAnnotator.h"
#include "Annotations/SymbolAnnotation.h"
#include "Annotations/ModuleAnnotation.h"

/**
 * Very complex annotator. It does things like:
 *  - Resolving the entire module header (which will be the annotation set in the module of type ModuleAnnotation).
 *  - Instructions and labels are still kept as children of the given module node, but they are annotated on their own.
 *
 * The module header is being annotated as:
 *  - Module return type.
 *  - Module symbol.
 *  - Module argument symbols.
 *
 * After annotation module header node is removed (flattening) and the annotation resulting from the resolution of it,
 * will be the one set in the module node.
 *
 * IMPORTANT: Every symbol except module's, will be registered on a new scope created JUST for this module.
 */
class ModuleAnnotator : public ScopeAbleAnnotator
{
  public:
    /**
     * Creates the module with the given symbol and type table.
     * @param symbolTable
     * @param typeTable
     */
    ModuleAnnotator(const ScopeAbleAnnotator::SymbolTableT &symbolTable,
                    const ScopeAbleAnnotator::TypeTableT &typeTable);

    /**
     * Annotates information to the given node. Returns true if succeeded. If it failed it will push errors to
     * the given logger and will return false.
     *
     * It assumes canAnnotate was called previously and it returned true.
     *
     * @param node
     * @param logger
     * @return bool
     */
    bool annotate(const std::shared_ptr<AstNode> &node, const std::shared_ptr<SourceLoggingSink> &logger) override;

    /**
     * Returns true if node and logger are valid; and if node is a module node.
     * @param node
     * @return bool
     */
    bool canAnnotate(const std::shared_ptr<AstNode> &node) override;

  private:
    /**
     * Annotates the header of the module and returns true if succeeded. It annotates:
     *  - Module return type.
     *  - Module symbol.
     *  - Arguments.
     *
     *  After annotation, every child node of the header node is removed.
     *
     *  Important: It also begins a Symbol scope AFTER module symbol has been declared. Caller must ensure calling
     *  endScope if this function returned true.
     * @param node
     * @param annot
     * @param logger
     * @return bool
     */
    bool annotateHeader(const std::shared_ptr<AstNode> &node,
                        const std::shared_ptr<ModuleAnnotation> &annot,
                        const std::shared_ptr<SourceLoggingSink> &logger);
};

#endif // EZPACKER_MODULEANNOTATOR_H
