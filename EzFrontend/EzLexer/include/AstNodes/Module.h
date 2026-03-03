#ifndef EZPACKER_MODULE_H
#define EZPACKER_MODULE_H

#include "EzLexerCommon.h"
#include "AstNode/AstNode.h"
#include "AstNode/AstNodeContainer.h"
#include "AstNode/AstNodeVisitor.h"
#include "AstNodes/CodeScope.h"
#include "AstNodes/Instruction.h"
#include "AstNodes/Label.h"

/**
 * This class represents the definition of a module, aka it's header. At the moment has little attributes but this
 * class is sensible to expansion.
 */
class ModuleHeader : public AstNode, public AstNodeContainer
{
  public:
    /**
     * Creates the module with the given name, return type and parameters.
     * @param parameters
     * @param moduleName
     * @param returnType
     */
    ModuleHeader(TypedPoolSlice<AstNode> *parameters, std::string_view moduleName, std::string_view returnType);

    /**
     * Returns the type of the node.
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
     * Returns the name of this AstNode.
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Returns the name of the module.
     * @return const std::string_view &
     */
    const std::string_view &getModuleName() const;

    /**
     * Returns the "return type" of the module.
     * @return const std::string &
     */
    const std::string_view &getReturnTypeName() const;

    /**
     * Returns this object in a formatted string (human readable). The quantity of the information included in the
     * formatted string depends on mode. If mode is set to default, only module return type and name will be shown.
     * Note: Debug mode currently builds parameter strings internally but does not include them in the output.
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;

  private:
    std::string_view m_moduleName;
    std::string_view m_returnType;
};

/**
 * This is the very first high-level node. A module is represented by its header and body, each one being a different
 * AstNode. This is a design decision to be able to change (if required) the internal structure of a module without
 * affecting its class directly.
 */
class Module : public AstNode
{
  public:
    /**
     * Creates the module with the given body and header.
     * @param body
     * @param header
     */
    Module(CodeScope *body, ModuleHeader *header);

    /**
     * Returns the type of the node.
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
     * Returns the body of the module.
     * @return CodeScope*
     */
    CodeScope *getBody() const;

    /**
     * Returns the name of this AstNode.
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Returns the header of this module.
     * @return ModuleHeader*
     */
    ModuleHeader *getHeader() const;

    /**
     * Returns this object in a formatted string (human readable). The quantity of the information included in the
     * formatted string depends on mode. Mode is passed into ModuleHeader and ModuleBody.
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;

  private:
    CodeScope *m_body;
    ModuleHeader *m_header;
};

#endif // EZPACKER_MODULE_H
