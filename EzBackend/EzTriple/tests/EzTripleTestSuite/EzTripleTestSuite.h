#ifndef EZPACKER_EZTRIPLETESTSUITE_H
#define EZPACKER_EZTRIPLETESTSUITE_H

#include "EzTriple.h"
#include "EzMirTestSuite.h"
/**
 * This class is used as a common entry point for triple tests. It takes 2 template parameters to be able to define
 * the current architecture we are stuck in.
 * @tparam TargetDescType
 * @tparam TargetLegalizerType
 */
template<typename TargetDescType, typename TargetLegalizerType>
class EzTripleTestSuite : public EzMirTestSuite
{
    public:
        /**
         * Returns the target descriptor of the selected triple.
         * @return TargetDescType
         */
        TargetDescType *getTargetDesc() const
        {
            return m_targetDesc.get();
        }

        /**
         * Returns a pointer to the legalizer of the selected triple.
         * @return TargetLegalizerType*
         */
        TargetLegalizerType *getLegalizer() const
        {
            return m_legalizer.get();
        }

        /**
         * Creates common pointers used in test cases. Also calls EzMirTestSuite::create.
         * @param workingPath
         */
        virtual void create(const std::filesystem::path &workingPath) override;

        /**
         * Frees everything of this test suite. Also calls EzMirTestSuite::destroy.
         */
        virtual void destroy() override;

    private:
        std::shared_ptr<TargetDescType> m_targetDesc;
        std::shared_ptr<TargetLegalizerType> m_legalizer;
};

#endif // EZPACKER_EZTRIPLETESTSUITE_H
