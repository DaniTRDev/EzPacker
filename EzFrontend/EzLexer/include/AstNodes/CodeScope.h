#ifndef EZPACKER_CODESCOPE_H
#define EZPACKER_CODESCOPE_H

#include "EzLexerCommon.h"
#include "AstNode/AstNode.h"
#include "AstNode/AstNodeContainer.h"

class CodeScope : public AstNode, public AstNodeContainer
{
  public:
    /**
     * Returns the CodeScope
     * @return AstNodeType
     */
    AstNodeType getType() const override;

    /**
     * Returns "CodeScope".
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Returns this object in a formatted string (human readable). The quantity of the information included in the
     * formatted string depends on mode. If mode is set to default, only instruction names will be shown. If mode is
     * set to debug, operands will also be shown.
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;
};

#endif // EZPACKER_CODESCOPE_H
