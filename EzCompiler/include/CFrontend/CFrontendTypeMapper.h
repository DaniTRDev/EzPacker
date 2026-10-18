#ifndef EZPACKER_C_FRONTEND_TYPE_MAPPER_H
#define EZPACKER_C_FRONTEND_TYPE_MAPPER_H

#include "EzCompilerCommon.h"

// Forward declarations
struct Type;
struct Node;
class MirType;
class MirTypeTable;
class MirBlock;

namespace EzCompiler::CFrontend
{

/**
 * Maps chibicc AST types (Type) to EzPacker SSA MIR types (MirType).
 * 
 * Supports primitives (void, bool, int8..int64, float, double),
 * pointer types, array/VLA types, and composite structures.
 */
MirType *mapType(MirTypeTable *tt, struct Type *ty);

/**
 * Determines whether the given MIR basic block ends with a terminator instruction.
 */
bool isBlockTerminated(const MirBlock *block);

} // namespace EzCompiler::CFrontend

#endif // EZPACKER_C_FRONTEND_TYPE_MAPPER_H

