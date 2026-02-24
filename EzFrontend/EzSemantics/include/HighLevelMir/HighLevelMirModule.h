#ifndef EZPACKER_HIGHLEVELMIRMODULE_H
#define EZPACKER_HIGHLEVELMIRMODULE_H

#include "EzSemanticsCommon.h"
#include "HighLevelMirBlock.h"

class HighLevelMirModule : public HighLevelMirBlock
{
  public:
    /**
     * Gets the parameter of this module. Returns nullptr if not found.
     * @param vRegId
     * @return HighLevelMirInstructionOperand
     */
    HighLevelMirInstructionOperand getParam(size_t vRegId);
    
    /**
     * Adds a parameter to the module.
     * @param param
     */
    void addParam(size_t vRegId, const HighLevelMirInstructionOperand &param);
    
    /**
     * Returns the list of parameters.
     * @return const std::vector<HighLevelMirInstructionOperand> &
     */
    const std::map<size_t, HighLevelMirInstructionOperand> &getParams() const;
    
    /**
     * Creates a module block.
     * @param id
     * @param parent
     * @return
     */
    static std::shared_ptr<HighLevelMirModule> create(size_t id, const std::shared_ptr<HighLevelMirBlock> &parent);

  private:
    // We need to store bit size and type: ex: call type checking (ensure the given parameters match the module decl).
    std::map<size_t, HighLevelMirInstructionOperand> m_parameters;
};

#endif // EZPACKER_HIGHLEVELMIRMODULE_H
