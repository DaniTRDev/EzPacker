#include <gtest/gtest.h>

#include "Operand/MirRegisterBank.h"
#include "Operand/MirRegisterClass.h"

/**
 * Verifies the register-descriptor hardware-encoding extension and sub-register aliasing,
 * the foundation consumed by generated register info.
 */
class MirRegisterDescriptorTest : public ::testing::Test
{
  protected:
    std::pmr::polymorphic_allocator<MirRegisterBank> bankAlloc{ &m_arena };
    std::pmr::polymorphic_allocator<MirRegisterClass> classAlloc{ &m_arena };

    std::pmr::monotonic_buffer_resource m_arena;
};

TEST_F(MirRegisterDescriptorTest, HardwareEncodingDefaultsToRegistrationOrder)
{
    auto *bank = bankAlloc.new_object<MirRegisterBank>("GPR", &m_arena);
    auto *cls = classAlloc.new_object<MirRegisterClass>("GPR64", bank, &m_arena);

    ASSERT_TRUE(cls->addRegister("rax", 64, 0, {}));
    ASSERT_TRUE(cls->addRegister("rcx", 64, 0, {}));

    EXPECT_EQ(cls->getReg("rax")->m_hwEncoding, 0u);
    EXPECT_EQ(cls->getReg("rax")->m_id, 0u);
    EXPECT_EQ(cls->getReg("rcx")->m_hwEncoding, 1u);
    EXPECT_EQ(cls->getReg("rcx")->m_id, 1u);
}

TEST_F(MirRegisterDescriptorTest, ExplicitHardwareEncodingOverridesRegistrationOrder)
{
    auto *bank = bankAlloc.new_object<MirRegisterBank>("GPR", &m_arena);
    auto *cls = classAlloc.new_object<MirRegisterClass>("GPR64", bank, &m_arena);

    ASSERT_TRUE(cls->addRegister("rax", 64, 0, {}, 0));
    ASSERT_TRUE(cls->addRegister("rcx", 64, 0, {}, 7));

    EXPECT_EQ(cls->getReg("rax")->m_hwEncoding, 0u);
    EXPECT_EQ(cls->getReg("rcx")->m_hwEncoding, 7u);
    EXPECT_EQ(cls->getReg("rcx")->m_id, 1u);
}

TEST_F(MirRegisterDescriptorTest, AddSubPartBuildsAliasingHierarchy)
{
    auto *bank = bankAlloc.new_object<MirRegisterBank>("GPR", &m_arena);
    auto *cls64 = classAlloc.new_object<MirRegisterClass>("GPR64", bank, &m_arena);
    auto *cls32 = classAlloc.new_object<MirRegisterClass>("GPR32", bank, &m_arena);
    auto *cls16 = classAlloc.new_object<MirRegisterClass>("GPR16", bank, &m_arena);

    ASSERT_TRUE(cls64->addRegister("rax", 64, 0, {}));
    ASSERT_TRUE(cls32->addRegister("eax", 32, 0, {}));
    ASSERT_TRUE(cls16->addRegister("ax", 16, 0, {}));

    auto *rax = cls64->getReg("rax");
    auto *eax = cls32->getReg("eax");
    auto *ax = cls16->getReg("ax");

    rax->addSubPart(eax);
    eax->addSubPart(ax);
    rax->addSubPart(nullptr);

    ASSERT_EQ(rax->m_subParts.size(), 1u);
    EXPECT_EQ(rax->m_subParts[0], eax);
    ASSERT_EQ(eax->m_subParts.size(), 1u);
    EXPECT_EQ(eax->m_subParts[0], ax);
}

TEST_F(MirRegisterDescriptorTest, DuplicateRegisterNamesAreRejected)
{
    auto *bank = bankAlloc.new_object<MirRegisterBank>("GPR", &m_arena);
    auto *cls = classAlloc.new_object<MirRegisterClass>("GPR64", bank, &m_arena);

    EXPECT_TRUE(cls->addRegister("rax", 64, 0, {}));
    EXPECT_FALSE(cls->addRegister("rax", 64, 0, {}));
}
