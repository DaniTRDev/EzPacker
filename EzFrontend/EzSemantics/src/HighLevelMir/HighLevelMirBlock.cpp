#include "HighLevelMir/HighLevelMirBlock.h"

std::shared_ptr<HighLevelMirBlock> HighLevelMirBlock::create(size_t id,
                                                             const std::shared_ptr<HighLevelMirBlock> &parent)
{
    std::shared_ptr<HighLevelMirBlock> result = std::make_shared<HighLevelMirBlock>();
    result->m_previous = parent;
    result->m_id = id;

    return result;
}
