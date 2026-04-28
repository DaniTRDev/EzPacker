#ifndef EZPACKER_MIRPASSMANAGER_H
#define EZPACKER_MIRPASSMANAGER_H

#include "EzMirCommon.h"
#include "IMirPass.h"

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

    template <typename T, typename... Args> T &getAnalysis(MirFunction *func, Args &&...args)
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
            throw std::runtime_error("Attempted to require a Transform pass as an Analysis");
        }

        analysisPass->run(func, this);

        T *passPtr = analysisPass.get();
        m_validAnalyses[typeId] = passPtr;

        m_cachedPasses.push_back(std::move(analysisPass));

        return *passPtr;
    }

    bool run(MirFunction *func)
    {
        for (auto &pass : m_passes)
        {
            if (!pass->run(func, this))
            {
                return false;
            }

            if (pass->getPassType() == MirPassType::Transform)
            {
                invalidateAllAnalyses();
            }
        }
        return true;
    }

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
