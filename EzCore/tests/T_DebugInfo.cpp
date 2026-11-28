#include "Debug/DebugInfo.h"
#include "Rtti/RttiDescriptor.h"

#include <gtest/gtest.h>

using namespace EzCore;
using namespace EzCore::Debug;

TEST(RttiDescriptorTest, AggregateInitializerBackwardsCompatibility)
{
    const RttiTypeDescriptor designated = {
        .typeId = kDefaultExceptionTypeId,
        .typeName = kDefaultExceptionTypeName,
        .numBases = 0,
        .bases = nullptr,
        .declarationSite = { "<runtime>", "<builtin>", 0, 0, "Default exception descriptor" }
    };

    EXPECT_EQ(designated.typeId, kDefaultExceptionTypeId);
    EXPECT_EQ(designated.typeName, kDefaultExceptionTypeName);
    EXPECT_EQ(designated.numBases, 0u);
    EXPECT_EQ(designated.bases, nullptr);
    EXPECT_EQ(designated.sizeInBytes, 0u);
    EXPECT_EQ(designated.alignment, 0u);
    EXPECT_EQ(designated.kind, RttiTypeKind::Unknown);
    EXPECT_EQ(designated.numMembers, 0u);
    EXPECT_EQ(designated.members, nullptr);

    const RttiTypeDescriptor positional = {
        computeTypeId("MyError"),
        "MyError",
        0,
        nullptr,
        { "err.c", "fn", 10, 2, "err" }
    };
    EXPECT_EQ(positional.typeName, "MyError");
    EXPECT_EQ(positional.kind, RttiTypeKind::Unknown);
    EXPECT_TRUE(designated.isA(&designated));
    EXPECT_FALSE(designated.isA(&positional));
}

TEST(RttiDescriptorTest, ExtendedMemberDescriptors)
{
    static constexpr RttiMemberDescriptor members[2] = {
        { "x", computeTypeId("int"), 0, 0, 0 },
        { "flags", computeTypeId("unsigned"), 4, 0, 3 }
    };

    const RttiTypeDescriptor structDesc = {
        .typeId = computeTypeId("Point"),
        .typeName = "Point",
        .numBases = 0,
        .bases = nullptr,
        .declarationSite = {},
        .sizeInBytes = 8,
        .alignment = 4,
        .kind = RttiTypeKind::Struct,
        .numMembers = 2,
        .members = members
    };

    EXPECT_EQ(structDesc.kind, RttiTypeKind::Struct);
    EXPECT_EQ(structDesc.sizeInBytes, 8u);
    EXPECT_EQ(structDesc.alignment, 4u);
    ASSERT_EQ(structDesc.numMembers, 2u);
    EXPECT_EQ(structDesc.members[0].name, "x");
    EXPECT_EQ(structDesc.members[1].bitWidth, 3u);
}

TEST(DebugInfoTest, SplitDirectoryAndFilename)
{
    std::string dir;
    std::string file;

    splitDirectoryAndFilename("/home/user/src/main.c", dir, file);
    EXPECT_EQ(dir, "/home/user/src");
    EXPECT_EQ(file, "main.c");

    splitDirectoryAndFilename("/main.c", dir, file);
    EXPECT_EQ(dir, "/");
    EXPECT_EQ(file, "main.c");

    splitDirectoryAndFilename("main.c", dir, file);
    EXPECT_TRUE(dir.empty());
    EXPECT_EQ(file, "main.c");

    splitDirectoryAndFilename("C:\\Users\\dev\\main.c", dir, file);
    EXPECT_EQ(dir, "C:\\Users\\dev");
    EXPECT_EQ(file, "main.c");

    splitDirectoryAndFilename("C:\\main.c", dir, file);
    EXPECT_EQ(dir, "C:\\");
    EXPECT_EQ(file, "main.c");

    splitDirectoryAndFilename("", dir, file);
    EXPECT_TRUE(dir.empty());
    EXPECT_TRUE(file.empty());
}

TEST(DebugInfoTest, AddOrGetFileDeduplicationAndIds)
{
    DebugCompileUnit cu;

    EXPECT_EQ(cu.addOrGetFile(""), 0u);

    const uint32_t id1 = cu.addOrGetFile("/workspace/proj/src/main.c");
    EXPECT_EQ(id1, 1u);

    const uint32_t id1Dup = cu.addOrGetFile("/workspace/proj/src/main.c");
    EXPECT_EQ(id1Dup, 1u);

    const uint32_t id2 = cu.addOrGetFile("src\\util\\helper.c");
    EXPECT_EQ(id2, 2u);

    const uint32_t id2Linux = cu.addOrGetFile("src/util/helper.c");
    EXPECT_EQ(id2Linux, 2u);

    ASSERT_EQ(cu.files.size(), 2u);
    EXPECT_EQ(cu.files[0].directory, "/workspace/proj/src");
    EXPECT_EQ(cu.files[0].filename, "main.c");
    EXPECT_EQ(cu.files[1].directory, "src/util");
    EXPECT_EQ(cu.files[1].filename, "helper.c");
    EXPECT_EQ(cu.files[1].fullPath, "src/util/helper.c");
}

TEST(DebugInfoTest, AddAndFindType)
{
    DebugCompileUnit cu;
    const uint64_t i32Id = computeTypeId("int32_t");

    DebugType i32;
    i32.typeId = i32Id;
    i32.name = "int";
    i32.kind = RttiTypeKind::SignedInt;
    i32.encoding = DebugBaseEncoding::Signed;
    i32.sizeInBytes = 4;
    i32.alignment = 4;

    cu.addType(i32);
    const DebugType *found = cu.findType(i32Id);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, "int");
    EXPECT_EQ(found->sizeInBytes, 4u);

    // Update existing typeId.
    i32.name = "int32_t";
    cu.addType(i32);
    EXPECT_EQ(cu.types.size(), 1u);
    found = cu.findType(i32Id);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, "int32_t");

    EXPECT_EQ(cu.findType(123456789ULL), nullptr);
}

TEST(DebugInfoTest, ValidateCompileUnit)
{
    DebugCompileUnit cu;
    std::string err;

    EXPECT_TRUE(cu.validate(err));
    EXPECT_TRUE(err.empty());

    const uint32_t fileId = cu.addOrGetFile("src/main.c");
    DebugType t;
    t.typeId = computeTypeId("int");
    t.name = "int";
    t.kind = RttiTypeKind::SignedInt;
    t.declLoc = { fileId, 1, 1 };
    cu.addType(t);

    DebugFunction fn;
    fn.name = "main";
    fn.declLoc = { fileId, 3, 1 };
    fn.lineEntries.push_back({ 0, fileId, 4, 5, true, false, false });
    cu.functions.push_back(fn);

    DebugGlobalVar gv;
    gv.name = "g_counter";
    gv.typeId = t.typeId;
    gv.declLoc = { fileId, 2, 1 };
    cu.globals.push_back(gv);

    EXPECT_TRUE(cu.validate(err));

    // Invalid pointerSize.
    cu.pointerSize = 3;
    EXPECT_FALSE(cu.validate(err));
    EXPECT_FALSE(err.empty());
    cu.pointerSize = 8;

    // Duplicate fileId.
    cu.files.push_back({ fileId, "src", "dup.c", "src/dup.c" });
    EXPECT_FALSE(cu.validate(err));
    cu.files.pop_back();

    // Unknown fileId in line entry.
    cu.functions[0].lineEntries[0].fileId = 999;
    EXPECT_FALSE(cu.validate(err));
    cu.functions[0].lineEntries[0].fileId = fileId;

    // Empty function name.
    cu.functions[0].name.clear();
    EXPECT_FALSE(cu.validate(err));
    cu.functions[0].name = "main";

    // Zero typeId.
    cu.types[0].typeId = 0;
    EXPECT_FALSE(cu.validate(err));
}

TEST(DebugInfoTest, RegisterMappings)
{
    // x86-64 DWARF GPRs
    EXPECT_EQ(mapX86_64ToDwarfReg(0, false), 0u);   // RAX -> 0
    EXPECT_EQ(mapX86_64ToDwarfReg(1, false), 2u);   // RCX -> 2
    EXPECT_EQ(mapX86_64ToDwarfReg(2, false), 1u);   // RDX -> 1
    EXPECT_EQ(mapX86_64ToDwarfReg(3, false), 3u);   // RBX -> 3
    EXPECT_EQ(mapX86_64ToDwarfReg(4, false), 7u);   // RSP -> 7
    EXPECT_EQ(mapX86_64ToDwarfReg(5, false), 6u);   // RBP -> 6
    EXPECT_EQ(mapX86_64ToDwarfReg(6, false), 4u);   // RSI -> 4
    EXPECT_EQ(mapX86_64ToDwarfReg(7, false), 5u);   // RDI -> 5
    EXPECT_EQ(mapX86_64ToDwarfReg(8, false), 8u);   // R8 -> 8
    EXPECT_EQ(mapX86_64ToDwarfReg(15, false), 15u); // R15 -> 15
    EXPECT_EQ(mapX86_64ToDwarfReg(16, false), 0xFFFFu);

    // x86-64 DWARF FPRs
    EXPECT_EQ(mapX86_64ToDwarfReg(0, true), 17u);  // XMM0 -> 17
    EXPECT_EQ(mapX86_64ToDwarfReg(15, true), 32u); // XMM15 -> 32
    EXPECT_EQ(mapX86_64ToDwarfReg(32, true), 0xFFFFu);

    // RISC-V 64 DWARF
    EXPECT_EQ(mapRiscV64ToDwarfReg(0, false), 0u);
    EXPECT_EQ(mapRiscV64ToDwarfReg(2, false), 2u);   // sp
    EXPECT_EQ(mapRiscV64ToDwarfReg(31, false), 31u);
    EXPECT_EQ(mapRiscV64ToDwarfReg(0, true), 32u);   // f0
    EXPECT_EQ(mapRiscV64ToDwarfReg(31, true), 63u);  // f31
    EXPECT_EQ(mapRiscV64ToDwarfReg(32, false), 0xFFFFu);
    EXPECT_EQ(mapRiscV64ToDwarfReg(32, true), 0xFFFFu);

    // x86-64 CodeView
    EXPECT_EQ(mapX86_64ToCodeViewReg(0, false, 8), 328u);  // RAX
    EXPECT_EQ(mapX86_64ToCodeViewReg(4, false, 8), 332u);  // RSP
    EXPECT_EQ(mapX86_64ToCodeViewReg(15, false, 8), 343u); // R15
    EXPECT_EQ(mapX86_64ToCodeViewReg(0, false, 4), 17u);   // EAX
    EXPECT_EQ(mapX86_64ToCodeViewReg(8, false, 4), 360u);  // R8D
    EXPECT_EQ(mapX86_64ToCodeViewReg(0, false, 2), 9u);    // AX
    EXPECT_EQ(mapX86_64ToCodeViewReg(8, false, 2), 352u);  // R8W
    EXPECT_EQ(mapX86_64ToCodeViewReg(0, false, 1), 1u);    // AL
    EXPECT_EQ(mapX86_64ToCodeViewReg(4, false, 1), 327u);  // SPL
    EXPECT_EQ(mapX86_64ToCodeViewReg(8, false, 1), 344u);  // R8B
    EXPECT_EQ(mapX86_64ToCodeViewReg(0, true, 16), 154u);  // XMM0
    EXPECT_EQ(mapX86_64ToCodeViewReg(8, true, 16), 252u);  // XMM8
    EXPECT_EQ(mapX86_64ToCodeViewReg(16, false, 8), 0u);   // NOREG
}
