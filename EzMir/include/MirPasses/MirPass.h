#ifndef EZMIR_MIR_PASS_H
#define EZMIR_MIR_PASS_H

#include "EzMirCommon.h"

enum class MirPassType : uint8_t
{
    Analysis, // Only reads the MIR.
    Transform // Can apply changes to the MIR.
};

enum class MirPassIterationPlace : uint8_t
{
    Function,       // The pass runs on each function.
    Block,          // The pass runs on each block.
    Instruction,    // The pass runs on each instruction.
    GlobalVariable, // The pass runs on each global var defined.
};

struct MirPassResult
{
    bool m_modifiedMir{ false }; // Set to true if the pass modified the MIR.
    bool m_executed{ false };    // Set to true if the pass was actually run.
    bool m_succeeded{ false };   // Set to true of the pass was run and succeeded.
};

/**
 * Interface used by passes that iterate over the MIR.
 *
 * TODO: Add MirModule level.
 */
class MirPass
{
  public:
    virtual ~MirPass() = default;

    /**
     * Runs the pass on the given MIR func.
     */
    virtual MirPassResult run(std::pmr::list<class MirFunction *> &funcList,
                              std::pmr::list<class MirFunction *>::iterator it,
                              class MirPassManager *passManager)
    {
        return {};
    }

    /**
     * Runs the pass on the given MIR block.
     */
    virtual MirPassResult run(std::pmr::list<class MirBlock *> &blockList,
                              std::pmr::list<class MirBlock *>::iterator it,
                              class MirPassManager *passManager)
    {
        return {};
    }

    /**
     * Runs the pass on the given MIR func.
     */
    virtual MirPassResult run(std::pmr::list<class MirInstruction *> &instrList,
                              std::pmr::list<class MirInstruction *>::iterator it,
                              class MirPassManager *passManager)
    {
        return {};
    }

    /**
     * Runs the pass on the given global variable. In this case the list/iterator is not needed as passes will only
     * modify things around the given variable.
     */
    virtual MirPassResult run(class MirGlobalVar *var, MirPassManager *passManager) { return {}; }

    /**
     * Returns the name of the pass.
     */
    virtual const char *getName() const = 0;

    /**
     * Returns the iteration place. Depending on the place, one callback or the other will be called.
     */
    virtual MirPassIterationPlace getIterationPlace() const = 0;

    /**
     * Returns the last result of this pass.
     */
    MirPassResult *getResult();

    /**
     * Returns the pass type.
     */
    virtual MirPassType getPassType() const = 0;

    /**
     * Prints the pass result to the diag collector.
     */
    virtual void printResult() {};

    /**
     * Called by the pass manager when the pass needs to be reset.
     */
    virtual void reset() {};

    /**
     * Sets the result of the pass (by copying the value, it does not store the pointer).
     */
    void setResult(MirPassResult *result);

    /**
     * Returns the dependencies linked to this pass (other passes that must be run before this one).
     */
    virtual std::vector<std::type_index> getDependencies() const { return {}; }

  private:
    MirPassResult m_result;
};

#endif // EZPACKER_MIRPASS_H
