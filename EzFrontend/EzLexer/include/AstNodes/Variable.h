/**
 * @file Variable.h
 * @brief AST node for variable references and declarations: `%name` or `type %name`.
 *
 * A Variable can represent a function parameter, a local variable created
 * with the `create` instruction, or a global variable.  The parser records
 * the data-type prefix (if present) and the variable name; the semantic
 * passes later resolve the symbol, validate types, and link the node to a
 * MIR virtual register.
 */
#ifndef EZPACKER_VARIABLE_H
#define EZPACKER_VARIABLE_H

#include "EzLexerCommon.h"
#include "AstNode/AstNodeContainer.h"
#include "AstNode/AstNodeVisitor.h"
#include "AstNodes/ImmediateOperand.h"

/**
 * This AstNode defines a global variable, local variable or function argument. The parser is blind about "where" the
 * definition of the variable is made. The semantic checker is the responsible of setting m_isLocal properly.
 */
class Variable : public AstNode, public AstNodeContainer
{
  public:
    /**
     * Creates the variable with the given data type, variable name and initializers. By default initializers are not
     * set (default parameters = {}). Also sets if this variable is an array or it isn't.
     * @param initializers
     * @param dataType
     * @param variableName
     */
    Variable(TypedPoolSlice<AstNode> *initializers, std::string_view dataType, std::string_view variableName);

    /**
     * Returns AstNodeType::Variable.
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
     * Returns if this variable is an array.
     * @return bool
     */
    bool getIsArray() const;

    /**
     * Returns the "Variable".
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Returns this object in a formatted string (human readable). The quantity of the information included in the
     * formatted string depends on mode. See AstNodeStringMode for more information. If mode is set to default, only
     * variable type and name will be shown. If mode is set to debug, initializers will also be included.
     * @param mode
     * @return std::string
     */
    std::string getAsStr(AstNodeStringMode mode) const override;

    /**
     * Returns the data-type name of this variable.
     * @return const std::string_view &
     */
    const std::string_view &getVariableDataType() const;

    /**
     * Returns the name of this variable
     * @return const std::string &
     */
    const std::string_view &getVariableName() const;

  private:
    bool m_isArray;
    std::string_view m_variableName;
    std::string_view m_variableDataType;
};

#endif // EZPACKER_VARIABLE_H
