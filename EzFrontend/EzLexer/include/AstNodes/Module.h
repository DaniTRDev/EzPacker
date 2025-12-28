#ifndef EZPACKER_MODULE_H
#define EZPACKER_MODULE_H

#include "EzLexerCommon.h"
#include "AstNodes/Instruction.h"
#include "AstNodes/Label.h"

/**
 * This class represents the definition of a module, aka it's header. At the moment has little attributes but this
 * class is sensible to expansion.
 */
class ModuleHeader : public AstNode
{
  public:
    /**
     * Creates the module with the given name, return type and parameters.
     * @param moduleName
     * @param returnType
     * @param parameters
     */
    ModuleHeader(std::string moduleName, std::string returnType, std::vector<std::shared_ptr<AstNode>> parameters);

    /**
     * Returns the type of the node.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

    /**
     * Returns the name of this AstNode.
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Returns the name of the module.
     * @return const std::string &
     */
    const std::string &getModuleName() const;

    /**
     * Returns the "return type" of the module.
     * @return const std::string &
     */
    const std::string &getReturnType() const;

    /**
     * Returns this object in a formatted string (human readable). The quantity of the information included in the
     * formatted string depends on mode. If mode is set to default, only module return type and name will be shwown.
     * If mode is set to debug, parameters will also be shown.
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;

    /**
     * Returns the parameters of the module.
     * @return const std::vector<std::shared_ptr<AstNode>> &
     */
    const std::vector<std::shared_ptr<AstNode>> &getParameters() const;

  private:
    std::string m_moduleName;
    std::string m_returnType;
    std::vector<std::shared_ptr<AstNode>> m_parameters;
};

/**
 * Type used to have a quick way of letting modules accept new expressions inside their body.
 */
using ModuleBodyExpr = std::variant<std::shared_ptr<Instruction>, std::shared_ptr<Label>>;
inline std::shared_ptr<AstNode> GetModuleExpressionAsNode(ModuleBodyExpr expression)
{
    if (std::holds_alternative<std::shared_ptr<Label>>(expression))
    {
        return std::get<std::shared_ptr<Label>>(expression);
    }
    else
    {
        return std::get<std::shared_ptr<Instruction>>(expression);
    }
    
    return nullptr;
}

/**
 * The content of a module. At the moment only instructions are supported but this might change in a near future.
 */
class ModuleBody : public AstNode
{
  public:
    /**
     * Creates the body with the given expressions.
     * @param expressions
     */
    ModuleBody(std::vector<ModuleBodyExpr> expressions);

    /**
     * Returns the type of the node.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

    /**
     * Returns the name of this AstNode.
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

    /**
     * Returns the instructions defined in the scope of this module.
     * @return const std::vector<ModuleBodyExpr> &
     */
    const std::vector<ModuleBodyExpr> &getExpressions() const;

  private:
    std::vector<ModuleBodyExpr> m_expressions;
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
    Module(std::shared_ptr<ModuleBody> body, std::shared_ptr<ModuleHeader> header);

    /**
     * Returns the type of the node.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

    /**
     * Returns the name of this AstNode.
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Returns the body of the module.
     * @return const std::shared_ptr<ModuleBodyParser> &
     */
    const std::shared_ptr<ModuleBody> &getBody() const;

    /**
     * Returns the header of a module
     * @return const std::shared_ptr<ModuleHeaderParser> &
     */
    const std::shared_ptr<ModuleHeader> &getHeader() const;

    /**
     * Returns this object in a formatted string (human readable). The quantity of the information included in the
     * formatted string depends on mode. Mode is passed into ModuleHeader and ModuleBody.
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;

  private:
    std::shared_ptr<ModuleBody> m_body;
    std::shared_ptr<ModuleHeader> m_header;
};

#endif // EZPACKER_MODULE_H
