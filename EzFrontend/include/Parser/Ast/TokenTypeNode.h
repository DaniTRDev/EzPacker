#ifndef EZPACKER_TOKENTYPENODE_H
#define EZPACKER_TOKENTYPENODE_H

#include "Ast.h"
#include "EzFrontendCommon.h"
#include "tokenizer/ITokenizer.h"

/**
 * This Ast node is of special interest. Its purpose is to define a node which content is retrieved directly from a
 * token. It's used when parser needs to get the exact content of a token to create the corresponding Ast:
 *  - IdentifierNode has a TokenTypeNode that contains the identifier.
 *  - ValueNode has a TokenTypeNode that contains the value (string, float, double, ...).
 *
 *  It will only be created in very specific scenarios. Look at grammar/TokenType to see how it is created.
 */
class TokenTypeNode : public Ast
{
  public:
    /**
     * Creates the object.
     */
    TokenTypeNode();
    
    /**
     * Returns the content of the Ast node.
     * @return const std::string &
     */
    const std::string &getContent();
    
    /**
     * Sets the content of the Ast node.
     */
    void setContent(const std::string &content);
    
    /**
     * Sets the token type.
     * @param type
     */
    void setTokenType(IRTokenType type);
    
    /**
     * Returns the type of the token.
     * @return IRTokenType
     */
    IRTokenType getTokenType() const;
    
    /**
     * Clones the object. Must be overridden by base classes. Returns nullptr if there was an error.
     * @return std::shared_ptr<Ast>
     */
    std::shared_ptr<Ast> clone() const override;
    
  private:
    std::string m_content;
    IRTokenType m_tokenType;
};

#endif // EZPACKER_TOKENTYPENODE_H
