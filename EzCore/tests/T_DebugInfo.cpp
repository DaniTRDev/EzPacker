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
    cu.types[0].typeId = t.typeId;

    // Dangling referencedTypeId in DebugType.
    DebugType ptrType;
    ptrType.typeId = computeTypeId("int*");
    ptrType.kind = RttiTypeKind::Pointer;
    ptrType.referencedTypeId = 999999ULL;
    cu.addType(ptrType);
    EXPECT_FALSE(cu.validate(err));
    ptrType.referencedTypeId = t.typeId;
    cu.addType(ptrType);
    EXPECT_TRUE(cu.validate(err));

    // Dangling member typeId in DebugType.
    DebugType structType;
    structType.typeId = computeTypeId("S");
    structType.kind = RttiTypeKind::Struct;
    structType.members.push_back({ "f", 888888ULL, 0, 0, 0, false });
    cu.addType(structType);
    EXPECT_FALSE(cu.validate(err));
    structType.members[0].typeId = t.typeId;
    cu.addType(structType);
    EXPECT_TRUE(cu.validate(err));

    // Dangling function returnTypeId and variable typeId.
    cu.functions[0].returnTypeId = 777777ULL;
    EXPECT_FALSE(cu.validate(err));
    cu.functions[0].returnTypeId = t.typeId;

    DebugVariable localVar;
    localVar.name = "x";
    localVar.typeId = 666666ULL;
    cu.functions[0].locals.push_back(localVar);
    EXPECT_FALSE(cu.validate(err));
    cu.functions[0].locals[0].typeId = t.typeId;
    EXPECT_TRUE(cu.validate(err));

    // Dangling global typeId.
    cu.globals[0].typeId = 555555ULL;
    EXPECT_FALSE(cu.validate(err));
    cu.globals[0].typeId = t.typeId;
    EXPECT_TRUE(cu.validate(err));

    // Non-monotonic lineEntries and codeOffset >= codeSize.
    cu.functions[0].codeSize = 32;
    cu.functions[0].lineEntries.push_back({ 16, fileId, 5, 1, true, false, false });
    cu.functions[0].lineEntries.push_back({ 8, fileId, 6, 1, true, false, false });
    EXPECT_FALSE(cu.validate(err));

    cu.functions[0].lineEntries[2].codeOffset = 32; // == codeSize (out of bounds)
    EXPECT_FALSE(cu.validate(err));

    cu.functions[0].lineEntries[2].codeOffset = 24;
    EXPECT_TRUE(cu.validate(err));
}

TEST(DebugInfoTest, IndexedLookupAndCopyability)
{
    DebugCompileUnit cu;
    constexpr size_t kCount = 2000;
    for (size_t i = 0; i < kCount; ++i)
    {
        const std::string path = "src/dir_" + std::to_string(i / 20) + "/file_" + std::to_string(i) + ".c";
        const uint32_t id = cu.addOrGetFile(path);
        EXPECT_EQ(id, static_cast<uint32_t>(i + 1));

        DebugType dt;
        dt.typeId = computeTypeId("type_" + std::to_string(i));
        dt.name = "type_" + std::to_string(i);
        dt.kind = RttiTypeKind::SignedInt;
        cu.addType(dt);
    }

    // Deduplication and updates still work in O(1).
    EXPECT_EQ(cu.addOrGetFile("src\\dir_0\\file_0.c"), 1u);
    EXPECT_EQ(cu.files.size(), kCount);

    const uint64_t midId = computeTypeId("type_1000");
    const DebugType *found = cu.findType(midId);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->name, "type_1000");

    // Copy and move preserve index consistency.
    DebugCompileUnit copied = cu;
    EXPECT_EQ(copied.addOrGetFile("src/dir_0/file_0.c"), 1u);
    ASSERT_NE(copied.findType(midId), nullptr);
    EXPECT_EQ(copied.findType(midId)->name, "type_1000");
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

    // AArch64 DWARF
    EXPECT_EQ(mapAArch64ToDwarfReg(0, false), 0u);
    EXPECT_EQ(mapAArch64ToDwarfReg(31, false), 31u);
    EXPECT_EQ(mapAArch64ToDwarfReg(0, true), 64u);
    EXPECT_EQ(mapAArch64ToDwarfReg(31, true), 95u);
    EXPECT_EQ(mapAArch64ToDwarfReg(32, false), 0xFFFFu);
    EXPECT_EQ(mapAArch64ToDwarfReg(32, true), 0xFFFFu);

    // Unified TargetArch dispatcher
    EXPECT_EQ(mapTargetRegToDwarfReg(DebugCompileUnit::TargetArch::X86_64, 4, false), 7u);
    EXPECT_EQ(mapTargetRegToDwarfReg(DebugCompileUnit::TargetArch::RiscV64, 2, true), 34u);
    EXPECT_EQ(mapTargetRegToDwarfReg(DebugCompileUnit::TargetArch::AArch64, 1, true), 65u);

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
