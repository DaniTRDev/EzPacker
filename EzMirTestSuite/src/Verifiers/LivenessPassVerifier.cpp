#include "Verifiers/LivenessPassVerifier.h"

LivenessAnalysisVerifier::LivenessAnalysisVerifier(LivenessAnalysisPass *pass) : MirPassVerifier(pass) {}

LivenessAnalysisVerifier &LivenessAnalysisVerifier::localDef(size_t blockId, size_t regId)
{
    const auto &res = getTestedObj()->getResult();
    auto it = res.m_def.find(blockId);

    EXPECT_NE(it, res.m_def.end());
    EXPECT_TRUE(it->second.contains(MirRegisterRef::vreg(regId)));

    return *this;
}

LivenessAnalysisVerifier &LivenessAnalysisVerifier::notLocalDef(size_t blockId, size_t regId)
{
    const auto &res = getTestedObj()->getResult();
    auto it = res.m_def.find(blockId);

    EXPECT_NE(it, res.m_def.end());
    EXPECT_FALSE(it->second.contains(MirRegisterRef::vreg(regId)));
    return *this;
}

LivenessAnalysisVerifier &LivenessAnalysisVerifier::localUse(size_t blockId, size_t regId)
{
    const auto &res = getTestedObj()->getResult();
    auto it = res.m_use.find(blockId);

    EXPECT_NE(it, res.m_use.end());
    EXPECT_TRUE(it->second.contains(MirRegisterRef::vreg(regId)));
    return *this;
}

LivenessAnalysisVerifier &LivenessAnalysisVerifier::notLocalUse(size_t blockId, size_t regId)
{
    const auto &res = getTestedObj()->getResult();
    auto it = res.m_use.find(blockId);

    EXPECT_NE(it, res.m_use.end());
    EXPECT_FALSE(it->second.contains(MirRegisterRef::vreg(regId)));
    return *this;
}

LivenessAnalysisVerifier &LivenessAnalysisVerifier::liveIn(size_t blockId, size_t regId)
{
    const auto &res = getTestedObj()->getResult();
    auto it = res.m_liveIn.find(blockId);

    EXPECT_NE(it, res.m_liveIn.end());
    EXPECT_TRUE(it->second.contains(MirRegisterRef::vreg(regId)));
    return *this;
}

LivenessAnalysisVerifier &LivenessAnalysisVerifier::notLiveIn(size_t blockId, size_t regId)
{
    const auto &res = getTestedObj()->getResult();
    auto it = res.m_liveIn.find(blockId);

    EXPECT_NE(it, res.m_liveIn.end());
    EXPECT_FALSE(it->second.contains(MirRegisterRef::vreg(regId)));
    return *this;
}

LivenessAnalysisVerifier &LivenessAnalysisVerifier::liveOut(size_t blockId, size_t regId)
{
    const auto &res = getTestedObj()->getResult();
    auto it = res.m_liveOut.find(blockId);

    EXPECT_NE(it, res.m_liveOut.end());
    EXPECT_TRUE(it->second.contains(MirRegisterRef::vreg(regId)));
    return *this;
}

LivenessAnalysisVerifier &LivenessAnalysisVerifier::notLiveOut(size_t blockId, size_t regId)
{
    const auto &res = getTestedObj()->getResult();
    auto it = res.m_liveOut.find(blockId);

    EXPECT_NE(it, res.m_liveOut.end());
    EXPECT_FALSE(it->second.contains(MirRegisterRef::vreg(regId)));
    return *this;
}

LivenessAnalysisVerifier &LivenessAnalysisVerifier::liveInCount(size_t blockId, size_t expectedCount)
{
    const auto &res = getTestedObj()->getResult();
    auto it = res.m_liveIn.find(blockId);

    EXPECT_NE(it, res.m_liveIn.end());
    EXPECT_EQ(it->second.size(), expectedCount);
    return *this;
}

LivenessAnalysisVerifier &LivenessAnalysisVerifier::liveOutCount(size_t blockId, size_t expectedCount)
{
    const auto &res = getTestedObj()->getResult();
    auto it = res.m_liveOut.find(blockId);

    EXPECT_NE(it, res.m_liveOut.end());
    EXPECT_EQ(it->second.size(), expectedCount);
    return *this;
}
