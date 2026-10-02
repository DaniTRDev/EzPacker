#include "EzLinkerTestSuite.h"

TEST_F(EzLinkerTestSuite, TestTryEnterAndLeave)
{
    EXPECT_EQ(__ez_get_top_frame(), nullptr);

    EzExceptionFrame frame1{};
    __ez_try_enter(&frame1);
    EXPECT_EQ(__ez_get_top_frame(), &frame1);

    EzExceptionFrame frame2{};
    __ez_try_enter(&frame2);
    EXPECT_EQ(__ez_get_top_frame(), &frame2);
    EXPECT_EQ(frame2.prev, &frame1);

    __ez_try_leave(&frame2);
    EXPECT_EQ(__ez_get_top_frame(), &frame1);

    __ez_try_leave(&frame1);
    EXPECT_EQ(__ez_get_top_frame(), nullptr);
}

TEST_F(EzLinkerTestSuite, TestCatchMatchesExact)
{
    EzCore::RttiTypeDescriptor typeA{
        .typeId = EzCore::computeTypeId("TypeA"),
        .typeName = "TypeA",
        .numBases = 0,
        .bases = nullptr
    };

    EXPECT_EQ(__ez_catch_matches(&typeA, &typeA), 1);
}

TEST_F(EzLinkerTestSuite, TestCatchMatchesCatchAll)
{
    EzCore::RttiTypeDescriptor typeA{
        .typeId = EzCore::computeTypeId("TypeA"),
        .typeName = "TypeA",
        .numBases = 0,
        .bases = nullptr
    };

    // nullptr filter represents catch (...) / catch-all
    EXPECT_EQ(__ez_catch_matches(&typeA, nullptr), 1);
    EXPECT_EQ(__ez_catch_matches(nullptr, nullptr), 1);
}

TEST_F(EzLinkerTestSuite, TestCatchMatchesNoRtti)
{
    EzCore::RttiTypeDescriptor filter{
        .typeId = EzCore::computeTypeId("Filter"),
        .typeName = "Filter"
    };

    // When thrown exception has no RTTI (--no-rtti), specific catch cannot match
    EXPECT_EQ(__ez_catch_matches(nullptr, &filter), 0);
}

TEST_F(EzLinkerTestSuite, TestCatchMatchesInheritance)
{
    EzCore::RttiTypeDescriptor baseDesc{
        .typeId = EzCore::computeTypeId("BaseException"),
        .typeName = "BaseException",
        .numBases = 0,
        .bases = nullptr
    };

    const EzCore::RttiTypeDescriptor *derivedBases[] = { &baseDesc };
    EzCore::RttiTypeDescriptor derivedDesc{
        .typeId = EzCore::computeTypeId("DerivedException"),
        .typeName = "DerivedException",
        .numBases = 1,
        .bases = derivedBases
    };

    // Derived matches Base
    EXPECT_EQ(__ez_catch_matches(&derivedDesc, &baseDesc), 1);

    // Base does not match Derived
    EXPECT_EQ(__ez_catch_matches(&baseDesc, &derivedDesc), 0);
}

static void helperThrow(void *payload, const void *rtti)
{
    __ez_throw(payload, rtti);
}

TEST_F(EzLinkerTestSuite, TestSjLjThrowAndCatch)
{
    EzCore::RttiTypeDescriptor exType{
        .typeId = EzCore::computeTypeId("CustomError"),
        .typeName = "CustomError"
    };

    int dummyPayload = 42;
    bool caught = false;

    EzExceptionFrame frame{};
    __ez_try_enter(&frame);

    if (setjmp(frame.jmpBuf) == 0)
    {
        // Try body: throw exception
        helperThrow(&dummyPayload, &exType);
        FAIL() << "Execution should not reach after helperThrow";
    }
    else
    {
        // Catch handler
        void *payload = __ez_get_current_exception();
        const void *rtti = __ez_get_current_rtti();

        EXPECT_EQ(payload, &dummyPayload);
        EXPECT_EQ(rtti, &exType);
        EXPECT_EQ(__ez_catch_matches(rtti, &exType), 1);
        caught = true;
    }

    __ez_try_leave(&frame);
    EXPECT_TRUE(caught);
    EXPECT_EQ(__ez_get_top_frame(), nullptr);
}

TEST_F(EzLinkerTestSuite, TestMultiCatch)
{
    EzCore::RttiTypeDescriptor typeA{
        .typeId = EzCore::computeTypeId("TypeA"),
        .typeName = "TypeA"
    };

    EzCore::RttiTypeDescriptor typeB{
        .typeId = EzCore::computeTypeId("TypeB"),
        .typeName = "TypeB"
    };

    int dummyPayload = 99;
    int matchedBranch = 0;

    EzExceptionFrame frame{};
    __ez_try_enter(&frame);

    if (setjmp(frame.jmpBuf) == 0)
    {
        helperThrow(&dummyPayload, &typeB);
    }
    else
    {
        const void *rtti = __ez_get_current_rtti();

        if (__ez_catch_matches(rtti, &typeA))
        {
            matchedBranch = 1;
        }
        else if (__ez_catch_matches(rtti, &typeB))
        {
            matchedBranch = 2;
        }
        else
        {
            matchedBranch = 3; // catch-all
        }
    }

    __ez_try_leave(&frame);
    EXPECT_EQ(matchedBranch, 2);
}

TEST_F(EzLinkerTestSuite, TestThrowWithoutRttiAttachesDefault)
{
    int dummyPayload = 777;
    bool caught = false;

    EzExceptionFrame frame{};
    __ez_try_enter(&frame);

    if (setjmp(frame.jmpBuf) == 0)
    {
        // Throw with NULL rtti
        __ez_throw(&dummyPayload, nullptr);
        FAIL() << "Execution should not reach after __ez_throw";
    }
    else
    {
        void *payload = __ez_get_current_exception();
        const void *rtti = __ez_get_current_rtti();

        EXPECT_EQ(payload, &dummyPayload);
        ASSERT_NE(rtti, nullptr);
        EXPECT_EQ(rtti, __ez_get_default_rtti());
        EXPECT_STREQ(__ez_get_rtti_type_name(rtti), "EzDefaultException");
        EXPECT_EQ(__ez_get_rtti_type_id(rtti), EzCore::kDefaultExceptionTypeId);
        caught = true;
    }

    __ez_try_leave(&frame);
    EXPECT_TRUE(caught);
    EXPECT_EQ(__ez_get_top_frame(), nullptr);
}

TEST_F(EzLinkerTestSuite, TestThrowNullPayloadUsesDefaultPayload)
{
    bool caught = false;

    EzExceptionFrame frame{};
    __ez_try_enter(&frame);

    if (setjmp(frame.jmpBuf) == 0)
    {
        // Throw with NULL payload and NULL rtti
        __ez_throw(nullptr, nullptr);
        FAIL() << "Execution should not reach after __ez_throw";
    }
    else
    {
        void *payload = __ez_get_current_exception();
        const void *rtti = __ez_get_current_rtti();

        ASSERT_NE(payload, nullptr);
        EXPECT_EQ(payload, __ez_get_default_payload());
        ASSERT_NE(rtti, nullptr);
        EXPECT_EQ(rtti, __ez_get_default_rtti());
        EXPECT_STREQ(__ez_get_rtti_type_name(rtti), "EzDefaultException");
        caught = true;
    }

    __ez_try_leave(&frame);
    EXPECT_TRUE(caught);
    EXPECT_EQ(__ez_get_top_frame(), nullptr);
}

TEST_F(EzLinkerTestSuite, TestRttiAccessors)
{
    EXPECT_STREQ(__ez_get_rtti_type_name(nullptr), "");
    EXPECT_EQ(__ez_get_rtti_type_id(nullptr), 0u);

    EzCore::RttiTypeDescriptor customDesc{
        .typeId = EzCore::computeTypeId("MyCustomException"),
        .typeName = "MyCustomException"
    };

    EXPECT_STREQ(__ez_get_rtti_type_name(&customDesc), "MyCustomException");
    EXPECT_EQ(__ez_get_rtti_type_id(&customDesc), EzCore::computeTypeId("MyCustomException"));

    const void *defaultRtti = __ez_get_default_rtti();
    EXPECT_STREQ(__ez_get_rtti_type_name(defaultRtti), "EzDefaultException");
    EXPECT_EQ(__ez_get_rtti_type_id(defaultRtti), EzCore::kDefaultExceptionTypeId);
}

TEST_F(EzLinkerTestSuite, TestCatchMatchesDefaultRtti)
{
    const void *defaultRtti = __ez_get_default_rtti();
    EXPECT_EQ(__ez_catch_matches(defaultRtti, defaultRtti), 1);
    EXPECT_EQ(__ez_catch_matches(defaultRtti, nullptr), 1); // catch-all

    EzCore::RttiTypeDescriptor otherFilter{
        .typeId = EzCore::computeTypeId("SomeOtherType"),
        .typeName = "SomeOtherType"
    };
    EXPECT_EQ(__ez_catch_matches(defaultRtti, &otherFilter), 0);
}

