#ifndef EZPACKER_EZTESTTRIPLEINSTRUCTIONSET_H
#define EZPACKER_EZTESTTRIPLEINSTRUCTIONSET_H

#include "EzTestTripleCommon.h"

namespace EzTestTriple
{
namespace TargetInst
{
#define TARGET_INSTRUCTION(name, str, id, ...) extern MirTargetInstructionDesc *name;
#include "InstructionSelector/EzTestTripleInstructionSetDefs.h"
#undef TARGET_INSTRUCTION

/// Helper to find a descriptor by integer ID (if needed for deserialization)
extern const MirTargetInstructionDesc *getById(size_t id);
} // namespace TargetInst
}; // namespace EzTestTriple

#endif // EZPACKER_EZTESTTRIPLEINSTRUCTIONSET_H