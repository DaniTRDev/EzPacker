#ifndef EZPACKER_MIRPASSMANAGER_H
#define EZPACKER_MIRPASSMANAGER_H

#include "EzMirCommon.h"
#include "IMirPass.h"
#include "Function/MirFunction.h"
#include "Printer/MirPrinter.h"

class MirPassManager
{
  public:
    /**
     * Adds the given pass to the list.
     * @tparam T
     * @tparam Args
     * @param args
     */
    template <typename T, typename... Args>
        requires(std::is_base_of<IMirPass, T>::value)
    void addPass(Args &&...args)
    {
        m_passes.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    template <typename T, typename IteratedElementType, typename... Args>
    T &getAnalysis(TypedPoolLinkedList<IteratedElementType> *list,
                   TypedPoolLinkedList<IteratedElementType>::Iterator it,
                   Args &&...args)
    {
        std::type_index typeId = std::type_index(typeid(T));

        // If the result is available and was not invalidated, return it.
        if (m_validAnalyses.contains(typeId))
        {
            return *static_cast<T *>(m_validAnalyses[typeId]);
        }

        // If no analysis result exists, create its pass and run it in-place.
        auto analysisPass = std::make_unique<T>(std::forward<Args...>(args)...);
        if (analysisPass->getPassType() != MirPassType::Analysis)
        {
            throw std::runtime_error("Attempted to require a Transform pass inside an Analysis");
        }

        analysisPass->run(list, it, this);

        T *passPtr = analysisPass.get();
        m_validAnalyses[typeId] = passPtr;

        m_cachedPasses.push_back(std::move(analysisPass));

        return *passPtr;
    }

    /**
     * Runs the pass on the given MIR func. Returns false if the list (or the elem inside the iterator) that holds the
     * iterator was modified.
     */
    bool run(TypedPoolLinkedList<class MirFunction> *funcList,
             TypedPoolLinkedList<class MirFunction>::Iterator it,
             class MirPassManager *passManager);

    /**
     * Runs the pass on the given MIR block. Returns false if the list (or the elem inside the iterator) that holds the
     * iterator was modified.
     */
    bool run(TypedPoolLinkedList<class MirBlock> *blockList,
             TypedPoolLinkedList<class MirBlock>::Iterator it,
             class MirPassManager *passManager);

    /**
     * Runs the pass on the given MIR func. Returns false if the list (or the elem inside the iterator) that holds the
     * iterator was modified.
     */
    bool run(TypedPoolLinkedList<class MirInstruction> *instrList,
             TypedPoolLinkedList<class MirInstruction>::Iterator it,
             class MirPassManager *passManager);

    void invalidateAllAnalyses()
    {
        m_validAnalyses.clear();
        m_cachedPasses.clear();
    }

  private:
    std::map<std::type_index, IMirPass *> m_validAnalyses;
    std::vector<std::unique_ptr<IMirPass>> m_cachedPasses;
    std::vector<std::unique_ptr<IMirPass>> m_passes;
};

#endif // EZPACKER_MIRPASSMANAGER_H
