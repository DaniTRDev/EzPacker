#ifndef EZPACKER_VARIABLE_H
#define EZPACKER_VARIABLE_H

#include "EzLexerCommon.h"
#include "AstNodes/ImmediateOperand.h"

/**
 * This AstNode defines a global variable, local variable or function argument. The parser is blind about "where" the
 * definition of the variable is made. The semantic checker is the responsible of setting m_isLocal properly.
 */
class Variable : public AstNode
{
  public:
    /**
     * Creates the variable with the given data type, variable name and initializers. By default initializers are not
     * set (default parameters = {}). Also sets if this variable is an array or it isn't.
     * @param isArray
     * @param dataType
     * @param variableName
     * @param initializers
     */
    Variable(bool isArray,
             std::string dataType,
             std::string variableName,
             std::vector<std::shared_ptr<AstNode>> initializers = {});

    /**
     * Returns AstNodeType::Variable.
     * @return AstNodeType
     */
    AstNodeType getType() const override;

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
     * @return const std::string &
     */
    const std::string &getVariableDataType() const;

    /**
     * Returns the name of this variable
     * @return const std::string &
     */
    const std::string &getVariableName() const;

    /**
     * Returns the initializers for the current variable.
     * @return const std::vector<std::shared_ptr<AstNode>> &
     */
    const std::vector<std::shared_ptr<AstNode>> &getInitializers() const;

  private:
    bool m_isArray;
    std::string m_variableName;
    std::string m_variableDataType;
    std::vector<std::shared_ptr<AstNode>> m_initializers;
};

#endif // EZPACKER_VARIABLE_H
