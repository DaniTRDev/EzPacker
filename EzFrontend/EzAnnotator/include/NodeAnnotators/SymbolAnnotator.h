#ifndef EZPACKER_SYMBOLANNOTATOR_H
#define EZPACKER_SYMBOLANNOTATOR_H

#include "EzAnnotatorCommon.h"
#include "ScopeAbleAnnotator.h"
#include "Annotations/SymbolAnnotation.h"

/**
 * This enum contains the different working modes of the SymbolAnnotator.
 */
enum class SymbolAnnotatorWorkingMode : uint8_t
{
    ExpectsExistingSymbol = 0, // Expects the symbol to exist IN ANY SCOPE, if it doesn't an error is returned. If it
                               // does, a new "reference" to the symbol is created. DEFAULT.
    CreateNewSymbol // Expects the symbol NOT to exist in THE CURRENT SCOPE, if it doesn't a new symbol is created. If
                    // it does, an error is returned.
};

/**
 * This annotator takes an identifier and creates a symbol of it, depending on its working mode (see
 * SymbolAnnotatorWorkingMode).
 *
 * After annotating, identifier's content is cleared (flattening).
 *
 * In many cases, this class acts as a helper for upper annotators. They use this class to get a symbolId, and then
 * they discard everything related to it (node, the annotation itself, ...) while they keep symbol and type IDs.
 */
class SymbolAnnotator : public ScopeAbleAnnotator
{
  public:
    /**
     * Creates the annotator with the given symbol table. Default working mode = ExpectsExistingSymbol.
     * @param symbolTable
     */
    SymbolAnnotator(const ScopeAbleAnnotator::SymbolTableT &symbolTable);

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
     * Returns true if node and logger are valid; and if the given node is an identifier.
     * @param node
     * @return bool
     */
    bool canAnnotate(const std::shared_ptr<AstNode> &node) override;

    /**
     * Sets the typeId that will be assigned to the annotated symbol.
     * @param typeId
     */
    void setTypeId(size_t typeId);

    /**
     * Sets the working mode for the annotator.
     * @param mode
     */
    void setWorkingMode(SymbolAnnotatorWorkingMode mode);

  private:
    SymbolAnnotatorWorkingMode m_workingMode;
    size_t m_typeId;
};

#endif // EZPACKER_SYMBOLANNOTATOR_H
