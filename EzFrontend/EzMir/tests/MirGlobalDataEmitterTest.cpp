#include "MirTestFixture.h"

// =============================================================================
//  createGlobalData – binary blobs
// =============================================================================

TEST_F(MirTestFixture, GlobalData_BinaryBlob)
{
    uint32_t data[] = { 0xDEADBEEF, 0xCAFEBABE };
    MirGlobalDataEntry *entry = m_globalDataEmitter->createGlobalData(data, sizeof(data), true);

    ASSERT_NE(entry, nullptr);
    EXPECT_TRUE(entry->m_isReadOnly);
    EXPECT_FALSE(entry->m_uninitialized);
    EXPECT_EQ(entry->m_dataSize, sizeof(data));
    EXPECT_NE(entry->m_entryId, 0);

    const uint32_t *entryData = reinterpret_cast<const uint32_t *>(entry->m_data.m_elems);
    EXPECT_EQ(entryData[0], 0xDEADBEEF);
    EXPECT_EQ(entryData[1], 0xCAFEBABE);
}

TEST_F(MirTestFixture, GlobalData_ReadWrite)
{
    uint64_t val = 42;
    MirGlobalDataEntry *entry = m_globalDataEmitter->createGlobalData(&val, sizeof(val), false);

    ASSERT_NE(entry, nullptr);
    EXPECT_FALSE(entry->m_isReadOnly);
    EXPECT_FALSE(entry->m_uninitialized);
    EXPECT_EQ(entry->m_dataSize, sizeof(val));

    uint64_t stored = *reinterpret_cast<const uint64_t *>(entry->m_data.m_elems);
    EXPECT_EQ(stored, 42);
}

TEST_F(MirTestFixture, GlobalData_NullData_MarkedUninitialized)
{
    MirGlobalDataEntry *entry = m_globalDataEmitter->createGlobalData(nullptr, 16, true);

    ASSERT_NE(entry, nullptr);
    EXPECT_TRUE(entry->m_uninitialized);
    EXPECT_EQ(entry->m_dataSize, 16);
    // Data should be zeroed
    for (size_t i = 0; i < 16; ++i)
    {
        EXPECT_EQ(entry->m_data.m_elems[i], 0);
    }
}

TEST_F(MirTestFixture, GlobalData_SingleByte)
{
    uint8_t byte = 0xAB;
    MirGlobalDataEntry *entry = m_globalDataEmitter->createGlobalData(&byte, sizeof(byte), true);

    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->m_dataSize, 1);
    EXPECT_EQ(entry->m_data.m_elems[0], 0xAB);
}

TEST_F(MirTestFixture, GlobalData_LargeBlob)
{
    constexpr size_t sz = 1024;
    uint8_t largeData[sz];
    for (size_t i = 0; i < sz; ++i)
        largeData[i] = static_cast<uint8_t>(i & 0xFF);

    MirGlobalDataEntry *entry = m_globalDataEmitter->createGlobalData(largeData, sz, true);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->m_dataSize, sz);

    for (size_t i = 0; i < sz; ++i)
    {
        EXPECT_EQ(entry->m_data.m_elems[i], static_cast<uint8_t>(i & 0xFF));
    }
}

TEST_F(MirTestFixture, GlobalData_MultipleEntries_DistinctIds)
{
    uint32_t a = 1, b = 2, c = 3;
    MirGlobalDataEntry *e1 = m_globalDataEmitter->createGlobalData(&a, sizeof(a), true);
    MirGlobalDataEntry *e2 = m_globalDataEmitter->createGlobalData(&b, sizeof(b), true);
    MirGlobalDataEntry *e3 = m_globalDataEmitter->createGlobalData(&c, sizeof(c), true);

    ASSERT_NE(e1, nullptr);
    ASSERT_NE(e2, nullptr);
    ASSERT_NE(e3, nullptr);
    EXPECT_NE(e1->m_entryId, e2->m_entryId);
    EXPECT_NE(e2->m_entryId, e3->m_entryId);
    EXPECT_NE(e1->m_entryId, e3->m_entryId);
}

TEST_F(MirTestFixture, GlobalData_DataIsCopied)
{
    uint64_t original = 0x1122334455667788;
    MirGlobalDataEntry *entry = m_globalDataEmitter->createGlobalData(&original, sizeof(original), true);
    ASSERT_NE(entry, nullptr);

    // Mutate the original – the entry should be unaffected
    original = 0;
    uint64_t stored = *reinterpret_cast<const uint64_t *>(entry->m_data.m_elems);
    EXPECT_EQ(stored, 0x1122334455667788);
}

// =============================================================================
//  createGlobalString
// =============================================================================

TEST_F(MirTestFixture, GlobalString_WithTerminator)
{
    const char *str = "Hello, World!";
    MirGlobalDataEntry *entry = m_globalDataEmitter->createGlobalString(str, true, true);

    ASSERT_NE(entry, nullptr);
    EXPECT_TRUE(entry->m_isReadOnly);
    EXPECT_FALSE(entry->m_uninitialized);
    EXPECT_EQ(entry->m_dataSize, strlen(str) + 1);
    EXPECT_STREQ(reinterpret_cast<const char *>(entry->m_data.m_elems), str);
}

TEST_F(MirTestFixture, GlobalString_WithoutTerminator)
{
    const char *str = "Hello, World!";
    MirGlobalDataEntry *entry = m_globalDataEmitter->createGlobalString(str, false, true);

    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->m_dataSize, strlen(str));
    EXPECT_EQ(strncmp(reinterpret_cast<const char *>(entry->m_data.m_elems), str, strlen(str)), 0);
}

TEST_F(MirTestFixture, GlobalString_Empty_WithTerminator)
{
    MirGlobalDataEntry *entry = m_globalDataEmitter->createGlobalString("", true, true);

    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->m_dataSize, 1); // just the '\0'
    EXPECT_EQ(entry->m_data.m_elems[0], '\0');
}

TEST_F(MirTestFixture, GlobalString_Empty_WithoutTerminator)
{
    MirGlobalDataEntry *entry = m_globalDataEmitter->createGlobalString("", false, true);

    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->m_dataSize, 0);
}

TEST_F(MirTestFixture, GlobalString_ReadWrite)
{
    MirGlobalDataEntry *entry = m_globalDataEmitter->createGlobalString("mutable", true, false);

    ASSERT_NE(entry, nullptr);
    EXPECT_FALSE(entry->m_isReadOnly);
}

TEST_F(MirTestFixture, GlobalString_LongString)
{
    std::string longStr(512, 'X');
    MirGlobalDataEntry *entry = m_globalDataEmitter->createGlobalString(longStr, true, true);

    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->m_dataSize, longStr.size() + 1);
    EXPECT_STREQ(reinterpret_cast<const char *>(entry->m_data.m_elems), longStr.c_str());
}

// =============================================================================
//  createGlobalInteger
// =============================================================================

TEST_F(MirTestFixture, GlobalInteger_Zero)
{
    MirGlobalDataEntry *entry = m_globalDataEmitter->createGlobalInteger(0);
    ASSERT_NE(entry, nullptr);
    EXPECT_TRUE(entry->m_isReadOnly);
    EXPECT_EQ(entry->m_dataSize, sizeof(uint64_t));

    uint64_t val = *reinterpret_cast<const uint64_t *>(entry->m_data.m_elems);
    EXPECT_EQ(val, 0);
}

TEST_F(MirTestFixture, GlobalInteger_Max)
{
    MirGlobalDataEntry *entry = m_globalDataEmitter->createGlobalInteger(UINT64_MAX);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->m_dataSize, sizeof(uint64_t));

    uint64_t val = *reinterpret_cast<const uint64_t *>(entry->m_data.m_elems);
    EXPECT_EQ(val, UINT64_MAX);
}

TEST_F(MirTestFixture, GlobalInteger_ArbitraryValue)
{
    MirGlobalDataEntry *entry = m_globalDataEmitter->createGlobalInteger(0xDEADBEEF);
    ASSERT_NE(entry, nullptr);

    uint64_t val = *reinterpret_cast<const uint64_t *>(entry->m_data.m_elems);
    EXPECT_EQ(val, 0xDEADBEEF);
}

// =============================================================================
//  createGlobalFloatingPoint
// =============================================================================

TEST_F(MirTestFixture, GlobalFloat_Zero)
{
    MirGlobalDataEntry *entry = m_globalDataEmitter->createGlobalFloatingPoint(0.0);
    ASSERT_NE(entry, nullptr);
    EXPECT_TRUE(entry->m_isReadOnly);
    EXPECT_EQ(entry->m_dataSize, sizeof(double));

    double val = *reinterpret_cast<const double *>(entry->m_data.m_elems);
    EXPECT_DOUBLE_EQ(val, 0.0);
}

TEST_F(MirTestFixture, GlobalFloat_Pi)
{
    MirGlobalDataEntry *entry = m_globalDataEmitter->createGlobalFloatingPoint(3.14159265358979);
    ASSERT_NE(entry, nullptr);

    double val = *reinterpret_cast<const double *>(entry->m_data.m_elems);
    EXPECT_DOUBLE_EQ(val, 3.14159265358979);
}

TEST_F(MirTestFixture, GlobalFloat_Negative)
{
    MirGlobalDataEntry *entry = m_globalDataEmitter->createGlobalFloatingPoint(-1.5);
    ASSERT_NE(entry, nullptr);

    double val = *reinterpret_cast<const double *>(entry->m_data.m_elems);
    EXPECT_DOUBLE_EQ(val, -1.5);
}

// =============================================================================
//  Emitter – getContext / attachToContext
// =============================================================================

TEST_F(MirTestFixture, GlobalDataEmitter_GetContext)
{
    EXPECT_EQ(m_globalDataEmitter->getContext(), m_context.get());
}
