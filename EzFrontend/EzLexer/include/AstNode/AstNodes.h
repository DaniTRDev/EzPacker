#ifndef EZPACKER_ASTNODES_H
#define EZPACKER_ASTNODES_H

#include "AstNode.h"

/**
 * FIXED: Nodes were previously declared as external global variables. This was a total mistake since these objects
 * would be processed in a translation unit and references to it would be processed on another, causing the "Static
 * Initialization Order Fiasco", which causes UB and accessing these objects without them being constructed.
 *
 * Fixed by making variables a function with a static-function-local variable that's returned.
 */

namespace AstNodes
{
AstNodeBuilder &Null();
AstNodeBuilder &Identifier();
AstNodeBuilder &Instruction();
AstNodeBuilder &InstructionOperand();
AstNodeBuilder &IntNumber();
AstNodeBuilder &FloatNumber();
AstNodeBuilder &Label();

AstNodeBuilder &BaseMemory();
AstNodeBuilder &BaseDisplMemory();
AstNodeBuilder &BaseIndexScaleDisplMemory();
AstNodeBuilder &DirectMemory();
AstNodeBuilder &IndexScaleMemory();

AstNodeBuilder &ModuleHeader();
AstNodeBuilder &Module();
AstNodeBuilder &String();
AstNodeBuilder &Type();
AstNodeBuilder &Variable();
AstNodeBuilder &VirtualVariable();
}; // namespace AstNodes

#endif // EZPACKER_ASTNODES_H
