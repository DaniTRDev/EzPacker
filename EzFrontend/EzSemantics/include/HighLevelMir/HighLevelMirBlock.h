#ifndef EZPACKER_HIGHLEVELMIRBLOCK_H
#define EZPACKER_HIGHLEVELMIRBLOCK_H

#include "EzSemanticsCommon.h"
#include "HighLevelMirInstruction.h"

/**
 * Class used as a container for MIR code.
 */
struct HighLevelMirBlock
{
    size_t m_id; // Id of this block. Will contain the SYMBOL ID of the symbol (module or label) that created this
                 // block.

    /**
     * Creates an empty node and sets its parent.
     * @param id
     * @param parent
     * @return std::shared_ptr<HighLevelMirBlock>
     */
    static std::shared_ptr<HighLevelMirBlock> create(size_t id, const std::shared_ptr<HighLevelMirBlock> &parent);

    std::shared_ptr<HighLevelMirBlock> m_next;
    std::shared_ptr<HighLevelMirBlock> m_previous;
    std::vector<HighLevelMirInstruction> m_instructions;
    std::vector<std::shared_ptr<SourceReference>> m_sourceReferences;
};

#endif // EZPACKER_HIGHLEVELMIRBLOCK_H
