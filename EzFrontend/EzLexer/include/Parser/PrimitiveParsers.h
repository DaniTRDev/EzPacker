#ifndef EZPACKER_PRIMITIVEPARSERS_H
#define EZPACKER_PRIMITIVEPARSERS_H

#include "TokenParsers.h"

/**
 * Parsers defined here represent the very basic primitive nodes an AST, excluding the nodes that come from tokens.
 * Nodes in this file should be the second lowest nodes in the AST: they must be parents of TokenParsers's results.
 */

namespace NodeParsers
{
using ParsingRule = std::shared_ptr<Rule>;
extern ParsingRule Keyword(const std::string &kw); // Parses a keyword ('.' + kw)
extern ParsingRule &Number();                      // Parses a number (int or float).
extern ParsingRule &Type();                        // Parses a type
extern ParsingRule &Variable();                    // Parses a variable (.variable name: .type initializers, ...)
extern ParsingRule &VirtualVariable();             // Parses a virtual variable ('%' + 'identifier')
} // namespace NodeParsers

#endif // EZPACKER_PRIMITIVEPARSERS_H
