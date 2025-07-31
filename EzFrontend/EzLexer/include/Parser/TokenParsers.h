#ifndef EZPACKER_TOKENPARSERS_H
#define EZPACKER_TOKENPARSERS_H

#include "AstNode/AstNodes.h"
#include "Rules.h"

/**
 * This file contains parsing rules that parse tokens and, in some cases (identifier, string, ...) save the content
 * of the token into an AST nodes. Parsers in this file represent the leafs of the AST.
 */

namespace NodeParsers
{
using ParsingRule = std::shared_ptr<Rule>;
/**
 * Expects given token in the current position of the ctx. ParsingRule position will be restored if token does not
 * match. If builder != nullptr, a new node will be created using the given builder and its content will be set with
 * token's.
 * @param type
 * @param copyContent
 * @param builder
 * @return std::shared_ptr<Rule>
 */
extern ParsingRule Token(_TokenType type, AstNodeBuilder *builder = nullptr);

extern ParsingRule &Colon();      // Parses a token of type ':'.
extern ParsingRule &Comma();      // Parses a token of type ','.
extern ParsingRule &Dot();        // Parses a token of type '.'.
extern ParsingRule &LeftBrace();  // Parses a token of type '{'.
extern ParsingRule &LeftParen();  // Parses a token of type '('.
extern ParsingRule &Percentage(); // Parses a token of type '%'.
extern ParsingRule &RightBrace(); // Parses a token of type '}'.
extern ParsingRule &RightParen(); // Parses a token of type ')'.
extern ParsingRule &SemiColon();  // Parses a token of type ';'.

extern ParsingRule &Identifier();  // Parses a token and throws an error if it's not present
extern ParsingRule &IntNumber();   // Parses an integer
extern ParsingRule &FloatNumber(); // Parses a floating-point number (any size: float, double, ...)
extern ParsingRule &String();      // Parses a string
} // namespace NodeParsers

#endif // EZPACKER_TOKENPARSERS_H
