#ifndef EZPACKER_INCLUDEASTNODE_H
#define EZPACKER_INCLUDEASTNODE_H

#include "EzLexerCommon.h"
#include "AstNode/AstNode.h"

class IncludeAstNode : public AstNode
{
  public:
    /**
     * Constructor. Initializes the object with the given included file relative path.
     * @param includedFileRelPath
     */
    IncludeAstNode(const std::string_view &includedFileRelPath);

    /**
     * Returns AstNodeType::Include.
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
     * Returns "IncludeAstNode".
     * @return const char*
     */
    const char *getAstNodeName() const override;
    
    /**
     * Returns the relative path of the file to be included.
     * @return const std::string_view &
     */
    const std::string_view &getIncludedFileRelPath() const;

  private:
    std::string_view m_includedFileRelPath;
};

#endif // EZPACKER_INCLUDEASTNODE_H
