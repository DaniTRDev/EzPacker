#ifndef EZPACKER_IDENTIFIERNODE_H
#define EZPACKER_IDENTIFIERNODE_H

#include "Ast.h"
#include "EzFrontendCommon.h"

class IdentifierNode : public Ast
{
  public:
    /**
     * Creates the object.
     */
    IdentifierNode();

    /**
     * Sets the identifier to the one given.
     * @param identifier
     */
    void setIdentifier(const std::string &identifier);

    /**
     * Clones the object. Must be overridden by base classes. Returns nullptr if there was an error.
     * @return std::shared_ptr<Ast>
     */
    std::shared_ptr<Ast> clone() const override;
    
    /**
     * Returns the identifier.
     * @return const std::string &
     */
    const std::string &getIdentifier();

  private:
    std::string m_identifier;
};

#endif // EZPACKER_IDENTIFIERNODE_H
