/**
 * @file Variable.h
 * @brief AST node for `%name` references and typed variable declarations.
 *
 * Variable nodes are reused by several parts of the grammar:
 * - plain variable references: `%value`
 * - typed references/declarations: `i64 %value`
 * - variable declarations with initializers: `i8 %bytes: {1, 2, 3}`
 *
 * The parser records only syntax. Questions such as "is this local, global,
 * parameter, or symbol use?" are answered later by EzSemantics.
 */
#ifndef EZPACKER_VARIABLE_H
#define EZPACKER_VARIABLE_H

#include "EzLexerCommon.h"
#include "AstNode/AstNodeContainer.h"
#include "AstNode/AstNodeVisitor.h"
#include "AstNodes/ImmediateOperand.h"

/**
 * Parsed variable-like construct.
 *
 * The inherited AstNodeContainer stores optional initializer expressions. When
 * present today, initializer entries are immediate values parsed from the
 * source. An empty or null initializer list means the variable was written
 * without `:` initialization syntax.
 */
class Variable : public AstNode, public AstNodeContainer
{
  public:
    /**
     * Creates a variable node.
     *
     * @param initializers Optional initializer slice. When this represents an
     *                     array initializer, the slice contains one entry per
     *                     element in source order.
     * @param dataType Parsed type spelling, or an empty string view when the
     *                 source omitted an explicit type.
     * @param variableName Variable name without the leading `%`.
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
     * Returns true when the parsed initializer used brace syntax and contains
     * more than one element.
     *
     * A single-element `{value}` initializer is represented as a non-array.
     */
    bool getIsArray() const;

    /**
     * Returns the "Variable".
     * @return const char*
     */
    const char *getAstNodeName() const override;

    /**
     * Returns the parsed data-type spelling.
     *
     * This may be empty for untyped variable references.
     */
    const std::string_view &getVariableDataType() const;

    /**
     * Returns the variable name without the leading `%`.
     */
    const std::string_view &getVariableName() const;

  private:
    bool m_isArray;
    std::string_view m_variableName;
    std::string_view m_variableDataType;
};

#endif // EZPACKER_VARIABLE_H
