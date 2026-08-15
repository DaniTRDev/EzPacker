#include "InstructionSelector/EzTestTripleInstructionSet.h"

namespace EzTestTriple
{
namespace TargetInst
{
namespace Storage
{
#define TARGET_INSTRUCTION(name, str, id, ...) static MirTargetInstructionDesc desc_##name{ str, id, __VA_ARGS__ };
#include "InstructionSelector/EzTestTripleInstructionSetDefs.h"
#undef TARGET_INSTRUCTION
}; // namespace Storage

#define TARGET_INSTRUCTION(name, str, id, ...) MirTargetInstructionDesc *name = &Storage::desc_##name;
#include "InstructionSelector/EzTestTripleInstructionSetDefs.h"
#undef TARGET_INSTRUCTION

const MirTargetInstructionDesc *getById(size_t id)
{
    static const std::unordered_map<size_t, MirTargetInstructionDesc *> idMap = {
#define TARGET_INSTRUCTION(name, str, id, ...) { id, TargetInst::name },
#include "InstructionSelector/EzTestTripleInstructionSetDefs.h"
#undef TARGET_INSTRUCTION
    };

    auto it = idMap.find(id);
    return (it != idMap.end()) ? it->second : nullptr;
}

}; // namespace TargetInst
} // namespace EzTestTriple