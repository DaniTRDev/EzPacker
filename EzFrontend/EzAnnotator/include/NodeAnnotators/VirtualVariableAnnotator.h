#ifndef EZPACKER_VIRTUALVARIABLEANNOTATOR_H
#define EZPACKER_VIRTUALVARIABLEANNOTATOR_H

#include "EzAnnotatorCommon.h"
#include "INodeAnnotator.h"
#include "NodeAnnotators/SymbolAnnotator.h"
#include "NodeAnnotators/ScopeAbleAnnotator.h"

/**
 * Used to know how a Virtual Variable should be annotated. This is basically the same behaviour as SymbolAnnotator.
 * But since I don't know if in a future I would need to add content to the SymbolAnnotator, its better to just
 * split. Makes code more composable and less dependant.
 */
enum class VVAnnotatorWorkingMode : uint8_t
{
    ExpectsExistingSymbol = 0, // Expects the VV to exist IN ANY SCOPE, if it doesn't an error is returned. If it
                               // does, a new "reference" to the VV is created. DEFAULT.
    CreateNewSymbol // Expects the VV NOT to exist in THE CURRENT SCOPE, if it doesn't a new VV is created. If
                    // it does, an error is returned.
};

/**
 * This annotator resolves virtual variable's into symbols. It allows setting the type that will be annotated with
 * the symbol and the working mode (see VVAnnotatorWorkingMode).
 */
class VirtualVariableAnnotator : public ScopeAbleAnnotator
{
  public:
    /**
     * Creates the annotator with the given type table.
     * @param symbolTable
     */
    VirtualVariableAnnotator(const ScopeAbleAnnotator::SymbolTableT &symbolTable);

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
     * Returns true if node and logger are valid; and if given node is a virtual variable node.
     * @param node
     * @return bool
     */
    bool canAnnotate(const std::shared_ptr<AstNode> &node) override;

    /**
     * Set the type ID for the next virtual variable that will be annotated.
     * @param typeId
     */
    void setTypeId(size_t typeId);

    /**
     * Sets the working mode for the annotator.
     * @param mode
     */
    void setWorkingMode(VVAnnotatorWorkingMode mode);

  private:
    size_t m_typeId; // Type that will be given to the virtual variable.
    VVAnnotatorWorkingMode m_workingMode;
};

#endif // EZPACKER_VIRTUALVARIABLEANNOTATOR_H
