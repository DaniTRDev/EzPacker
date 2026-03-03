#ifndef EZPACKER_CODESCOPE_H
#define EZPACKER_CODESCOPE_H

#include "EzLexerCommon.h"
#include "AstNode/AstNode.h"
#include "AstNode/AstNodeVisitor.h"
#include "AstNode/AstNodeContainer.h"

class CodeScope : public AstNode, public AstNodeContainer
{
  public:
    /**
     * Returns AstNodeType::CodeScope.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

    /**
     * Accepts the given visitor and calls its internal visit method with the correct node type. Returns
     * the result of visit.
     * @param visitor
     * @return bool
     */
    bool accept(AstNodeVisitor *visitor) override;

    /**
     * Returns "CodeScope".
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Returns this object in a formatted string (human readable). The quantity of the information included in the
     * formatted string depends on mode. If mode is set to default, only the expression count is shown (e.g.,
     * "CodeScope (size: N)"). If mode is set to debug, each contained expression is expanded with its own getAsStr
     * output.
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;
};

#endif // EZPACKER_CODESCOPE_H
