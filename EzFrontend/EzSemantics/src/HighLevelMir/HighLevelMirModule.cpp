#include "HighLevelMir/HighLevelMirModule.h"

HighLevelMirInstructionOperand HighLevelMirModule::getParam(size_t vRegId)
{
    auto it = m_parameters.find(vRegId);
    if (it == m_parameters.end())
        return {};

    return it->second;
}

void HighLevelMirModule::addParam(size_t vRegId, const HighLevelMirInstructionOperand &param)
{
    m_parameters.insert({ vRegId, param });
}

const std::map<size_t, HighLevelMirInstructionOperand> &HighLevelMirModule::getParams() const { return m_parameters; }

std::shared_ptr<HighLevelMirModule> HighLevelMirModule::create(size_t id,
                                                               const std::shared_ptr<HighLevelMirBlock> &parent)
{
    std::shared_ptr<HighLevelMirModule> module = std::make_shared<HighLevelMirModule>();
    module->m_id = id;
    std::dynamic_pointer_cast<HighLevelMirBlock>(module)->m_previous = parent;

    return module;
}
