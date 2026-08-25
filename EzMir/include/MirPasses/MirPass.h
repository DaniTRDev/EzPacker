#ifndef EZMIR_MIR_PASS_H
#define EZMIR_MIR_PASS_H

#include "EzMirCommon.h"
#include "HelperClasses/IntrusiveLinkedList.h"

/**
 * Classification category of a compiler pass (Analysis or Transform).
 */
enum class MirPassType : uint8_t
{
    Analysis, // Read-only pass computing metrics/dataflow without mutating the MIR
    Transform // Mutating pass modifying instruction sequences, CFGs, or operand layouts
};

/**
 * Granularity level at which a MIR compiler pass operates.
 */
enum class MirPassIterationPlace : uint8_t
{
    Function,       // The pass runs per MirFunction instance
    Block,          // The pass runs per MirBlock in each function
    Instruction,    // The pass runs per MirInstruction in each block
    GlobalVariable, // The pass runs per MirGlobalVar definition
};

/**
 * Status and modification flag bundle returned after executing a compiler pass.
 */
struct MirPassResult
{
    bool m_modifiedMir{ false }; // True if the pass mutated the MIR representation
    bool m_executed{ false };    // True if the pass was invoked
    bool m_succeeded{ false };   // True if the pass completed without fatal errors
};

/**
 * Base polymorphic interface for all MIR optimization, transformation, and analysis passes.
 */
class MirPass
{
  public:
    virtual ~MirPass() = default;

    /**
     * Executes the pass over an individual MirFunction in the intrusive function list.
     */
    virtual MirPassResult run(IntrusiveLinkedList<class MirFunction> &funcList,
                              IntrusiveLinkedList<class MirFunction>::iterator it,
                              class MirPassManager *passManager)
    {
        return {};
    }

    /**
     * Executes the pass over an individual MirBlock in the function's block chain.
     */
    virtual MirPassResult run(IntrusiveLinkedList<class MirBlock> &blockList,
                              IntrusiveLinkedList<class MirBlock>::iterator it,
                              class MirPassManager *passManager)
    {
        return {};
    }

    /**
     * Executes the pass over an individual MirInstruction in a block's instruction chain.
     */
    virtual MirPassResult run(IntrusiveLinkedList<class MirInstruction> &instrList,
                              IntrusiveLinkedList<class MirInstruction>::iterator it,
                              class MirPassManager *passManager)
    {
        return {};
    }

    /**
     * Executes the pass over a global variable definition.
     */
    virtual MirPassResult run(class MirGlobalVar *var, MirPassManager *passManager) { return {}; }

    /**
     * Returns the human-readable identifier name of this pass.
     */
    virtual const char *getName() const = 0;

    /**
     * Returns the iteration granularity level determining which run overload is invoked.
     */
    virtual MirPassIterationPlace getIterationPlace() const = 0;

    /**
     * Returns a pointer to the last execution result of this pass.
     */
    MirPassResult *getResult();

    /**
     * Returns the MirPassType discriminator (Analysis vs Transform).
     */
    virtual MirPassType getPassType() const = 0;

    /**
     * Formats and logs pass results to the attached diagnostic collector.
     */
    virtual void printResult() {};

    /**
     * Clears internal cached analysis or transform state for a subsequent compilation run.
     */
    virtual void reset() {};

    /**
     * Stores a copy of the given execution result.
     */
    void setResult(MirPassResult *result);

    /**
     * Returns the collection of pass type indices that must precede this pass in the pipeline.
     */
    virtual std::vector<std::type_index> getDependencies() const { return {}; }

  private:
    MirPassResult m_result;
};

#endif // EZPACKER_MIRPASS_H
