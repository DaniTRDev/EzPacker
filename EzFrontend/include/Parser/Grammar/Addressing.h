#ifndef EZPACKER_ADDRESSING_H
#define EZPACKER_ADDRESSING_H

#include "Combinators.h"
#include "EzFrontendCommon.h"
#include "Number.h"
#include "TokenType.h"
#include "Type.h"
#include "VirtualVariable.h"
#include "parser/ast/MemoryNode.h"

namespace grammar
{
/**
 * Creates a rule that tries to match base addressing mode. Returns MemoryNode if succeeded.
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> base();

/**
 * Creates a rule that tries to match base displ scale mode. Returns MemoryNode if succeeded.
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> baseDispl();

/**
 * Creates a rule that tries to match base index scale mode. Returns MemoryNode if succeeded.
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> baseIndexScaleDisplacement();

/**
 * Creates a rule that tries to match direct addressing mode. Returns MemoryNode if succeeded.
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> direct();

/**
 * Creates a rule that tries to match base index scale mode. Returns MemoryNode if succeeded.
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> indexScale();

/**
 * Creates a rule that tries to match with any of the rules above. Returns MemoryNode if succeeded.
 * @return std::shared_ptr<ParseRule>
 */
extern std::shared_ptr<ParseRule> memory();
} // namespace Grammar::addressing

#endif // EZPACKER_ADDRESSING_H
