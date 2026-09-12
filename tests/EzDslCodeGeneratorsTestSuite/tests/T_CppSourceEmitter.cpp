#include "EzDslCodeGeneratorsTestSuite.h"
#include "CodeGenerators/CppSourceEmitter.h"

using namespace CodeGenerators;

/**
 * Unit test suite for CppSourceEmitter formatting and syntax construction engine.
 */
class CppSourceEmitterTest : public ::testing::Test
{
};

// ============================================================================
// 1. Indentation Management
// ============================================================================

TEST_F(CppSourceEmitterTest, TestBasicIndentation)
{
    CppSourceEmitter emitter;
    EXPECT_EQ(emitter.getIndentLevel(), 0);

    emitter.emitLine("int a = 0;");
    emitter.indent();
    EXPECT_EQ(emitter.getIndentLevel(), 1);
    emitter.emitLine("int b = 1;");
    emitter.indent();
    EXPECT_EQ(emitter.getIndentLevel(), 2);
    emitter.emitLine("int c = 2;");
    emitter.dedent();
    EXPECT_EQ(emitter.getIndentLevel(), 1);
    emitter.emitLine("int d = 3;");
    emitter.dedent();
    EXPECT_EQ(emitter.getIndentLevel(), 0);
    emitter.emitLine("int e = 4;");

    std::string expected =
            "int a = 0;\n"
            "    int b = 1;\n"
            "        int c = 2;\n"
            "    int d = 3;\n"
            "int e = 4;\n";

    EXPECT_EQ(emitter.str(), expected);
}

TEST_F(CppSourceEmitterTest, TestCustomIndentString)
{
    CppSourceEmitter emitter(1024, "  "); // 2-space indentation
    EXPECT_EQ(emitter.getIndentString(), "  ");

    emitter.emitLine("root");
    emitter.indent();
    emitter.emitLine("level1");
    emitter.indent();
    emitter.emitLine("level2");

    emitter.setIndentString("\t"); // Switch to tab indentation
    EXPECT_EQ(emitter.getIndentString(), "\t");
    emitter.emitLine("tab_level2");
    emitter.dedent();
    emitter.emitLine("tab_level1");

    std::string expected =
            "root\n"
            "  level1\n"
            "    level2\n"
            "\t\ttab_level2\n"
            "\ttab_level1\n";

    EXPECT_EQ(emitter.str(), expected);
}

TEST_F(CppSourceEmitterTest, TestDedentBoundaryAtZeroLevel)
{
    CppSourceEmitter emitter;
    EXPECT_EQ(emitter.getIndentLevel(), 0);

    // Dedenting below 0 should be safely clamped to 0 without underflowing
    emitter.dedent();
    emitter.dedent();
    EXPECT_EQ(emitter.getIndentLevel(), 0);

    emitter.setIndentLevel(5);
    EXPECT_EQ(emitter.getIndentLevel(), 5);
    emitter.setIndentLevel(0);
    EXPECT_EQ(emitter.getIndentLevel(), 0);
}

// ============================================================================
// 2. Scoped Blocks and Declarations
// ============================================================================

TEST_F(CppSourceEmitterTest, TestNamespaceAndClassScopes)
{
    CppSourceEmitter emitter;

    {
        auto ns = emitter.enterNamespace("MyNamespace");
        {
            auto cls = emitter.enterClass("MyClass", "public Base");
            emitter.emitLine("public:");
            emitter.indent();
            emitter.emitLine("void foo();");
            emitter.dedent();
        }
    }

    std::string expected =
            "namespace MyNamespace\n"
            "{\n"
            "    class MyClass : public Base\n"
            "    {\n"
            "        public:\n"
            "            void foo();\n"
            "    };\n"
            "} // namespace MyNamespace\n";

    EXPECT_EQ(emitter.str(), expected);
}

TEST_F(CppSourceEmitterTest, TestClassAndStructWithoutBase)
{
    CppSourceEmitter emitter;

    {
        auto cls = emitter.enterClass("StandaloneClass");
        emitter.emitLine("int x{ 0 };");
    }
    emitter.emitBlankLine();
    {
        auto st = emitter.enterStruct("StandaloneStruct");
        emitter.emitLine("int y{ 0 };");
    }
    emitter.emitBlankLine();
    {
        auto stDerived = emitter.enterStruct("DerivedStruct", "BaseStruct");
        emitter.emitLine("int z{ 0 };");
    }

    std::string expected =
            "class StandaloneClass\n"
            "{\n"
            "    int x{ 0 };\n"
            "};\n"
            "\n"
            "struct StandaloneStruct\n"
            "{\n"
            "    int y{ 0 };\n"
            "};\n"
            "\n"
            "struct DerivedStruct : BaseStruct\n"
            "{\n"
            "    int z{ 0 };\n"
            "};\n";

    EXPECT_EQ(emitter.str(), expected);
}

TEST_F(CppSourceEmitterTest, TestEnumDeclarations)
{
    CppSourceEmitter emitter;

    // 1. Enum class with underlying type
    {
        auto enm = emitter.enterEnum("Status", "uint8_t", true);
        emitter.emitLine("Active = 1,");
        emitter.emitLine("Inactive = 2");
    }

    emitter.emitBlankLine();

    // 2. Enum class without underlying type
    {
        auto enm = emitter.enterEnum("Color", "", true);
        emitter.emitLine("Red,");
        emitter.emitLine("Green,");
        emitter.emitLine("Blue");
    }

    emitter.emitBlankLine();

    // 3. Legacy enum with underlying type
    {
        auto enm = emitter.enterEnum("Flags", "uint32_t", false);
        emitter.emitLine("FlagA = 0x1,");
        emitter.emitLine("FlagB = 0x2");
    }

    emitter.emitBlankLine();

    // 4. Legacy enum without underlying type
    {
        auto enm = emitter.enterEnum("SimpleEnum", "", false);
        emitter.emitLine("ValA,");
        emitter.emitLine("ValB");
    }

    std::string expected =
            "enum class Status : uint8_t\n"
            "{\n"
            "    Active = 1,\n"
            "    Inactive = 2\n"
            "};\n"
            "\n"
            "enum class Color\n"
            "{\n"
            "    Red,\n"
            "    Green,\n"
            "    Blue\n"
            "};\n"
            "\n"
            "enum Flags : uint32_t\n"
            "{\n"
            "    FlagA = 0x1,\n"
            "    FlagB = 0x2\n"
            "};\n"
            "\n"
            "enum SimpleEnum\n"
            "{\n"
            "    ValA,\n"
            "    ValB\n"
            "};\n";

    EXPECT_EQ(emitter.str(), expected);
}

TEST_F(CppSourceEmitterTest, TestPreprocessorConditionals)
{
    CppSourceEmitter emitter;

    {
        auto ifdef = emitter.enterIfdef("ENABLE_FEATURE_A");
        emitter.emitLine("void featureA();");
    }

    emitter.emitBlankLine();

    {
        auto ifndef = emitter.enterIfndef("DISABLE_FEATURE_B");
        emitter.emitLine("void featureB();");
    }

    std::string expected =
            "#ifdef ENABLE_FEATURE_A\n"
            "void featureA();\n"
            "#endif // ENABLE_FEATURE_A\n"
            "\n"
            "#ifndef DISABLE_FEATURE_B\n"
            "void featureB();\n"
            "#endif // DISABLE_FEATURE_B\n";

    EXPECT_EQ(emitter.str(), expected);
}

TEST_F(CppSourceEmitterTest, TestEnterBlockWithAndWithoutPrefix)
{
    CppSourceEmitter emitter;

    {
        auto b1 = emitter.enterBlock("if (condition)");
        emitter.emitLine("doSomething();");
    }

    emitter.emitBlankLine();

    {
        auto b2 = emitter.enterBlock();
        emitter.emitLine("anonymousScope();");
    }

    std::string expected =
            "if (condition)\n"
            "{\n"
            "    doSomething();\n"
            "}\n"
            "\n"
            "{\n"
            "    anonymousScope();\n"
            "}\n";

    EXPECT_EQ(emitter.str(), expected);
}

// ============================================================================
// 3. Scope RAII Lifetime, Explicit Close, and Move Semantics
// ============================================================================

TEST_F(CppSourceEmitterTest, TestScopeExplicitClose)
{
    CppSourceEmitter emitter;

    {
        auto sc = emitter.enterScope("void test() {", "}", true);
        emitter.emitLine("return;");
        sc.close(); // Explicitly close before end of block
        // Subsequent destructor execution should be a no-op
    }

    std::string expected =
            "void test() {\n"
            "    return;\n"
            "}\n";

    EXPECT_EQ(emitter.str(), expected);
    EXPECT_EQ(emitter.getIndentLevel(), 0);
}

TEST_F(CppSourceEmitterTest, TestScopeMoveConstruction)
{
    CppSourceEmitter emitter;

    {
        auto sc1 = emitter.enterBlock("while (true)");
        emitter.emitLine("step1();");

        // Move construct sc2 from sc1
        auto sc2 = std::move(sc1);
        emitter.emitLine("step2();");
    }

    std::string expected =
            "while (true)\n"
            "{\n"
            "    step1();\n"
            "    step2();\n"
            "}\n";

    EXPECT_EQ(emitter.str(), expected);
    EXPECT_EQ(emitter.getIndentLevel(), 0);
}

TEST_F(CppSourceEmitterTest, TestScopeMoveAssignment)
{
    CppSourceEmitter emitter;

    {
        auto sc1 = emitter.enterBlock("if (a)");
        emitter.emitLine("actionA();");

        auto sc2 = emitter.enterBlock("if (b)");
        emitter.emitLine("actionB();");

        // Move assignment: sc2 is overwritten by sc1. sc2 closes its block first.
        sc2 = std::move(sc1);
    }

    std::string expected =
            "if (a)\n"
            "{\n"
            "    actionA();\n"
            "    if (b)\n"
            "    {\n"
            "        actionB();\n"
            "    }\n"
            "}\n";

    EXPECT_EQ(emitter.str(), expected);
    EXPECT_EQ(emitter.getIndentLevel(), 0);
}

// ============================================================================
// 4. Blank Line Handling & Deduplication
// ============================================================================

TEST_F(CppSourceEmitterTest, TestBlankLineDeduplication)
{
    CppSourceEmitter emitter;
    emitter.emitBlankLine(); // Empty buffer - should not emit anything
    EXPECT_TRUE(emitter.empty());

    emitter.emitLine("line 1;");
    emitter.emitBlankLine();
    emitter.emitBlankLine(); // Redundant blank line - should be deduplicated
    emitter.emitBlankLine();
    emitter.emitLine("line 2;");

    std::string expected =
            "line 1;\n"
            "\n"
            "line 2;\n";

    EXPECT_EQ(emitter.str(), expected);
}

// ============================================================================
// 5. Multiline and Line Ending Normalization
// ============================================================================

TEST_F(CppSourceEmitterTest, TestMultilineUnixAndWindowsLineEndings)
{
    CppSourceEmitter emitter;
    emitter.indent();
    emitter.emitLines("first line\nsecond line\r\nthird line\r\nfourth line");

    std::string expected =
            "    first line\n"
            "    second line\n"
            "    third line\n"
            "    fourth line\n";

    EXPECT_EQ(emitter.str(), expected);
}

TEST_F(CppSourceEmitterTest, TestMultilineTrailingNewline)
{
    CppSourceEmitter emitter;
    emitter.emitLines("line 1\nline 2\n");

    std::string expected =
            "line 1\n"
            "line 2\n";

    EXPECT_EQ(emitter.str(), expected);
}

// ============================================================================
// 6. Formatted, Partial, and Raw Line Emission
// ============================================================================

TEST_F(CppSourceEmitterTest, TestFormattedAndPartialEmission)
{
    CppSourceEmitter emitter;
    emitter.indent();

    emitter.emitLine("int value = {};", 42);

    // Partial emission without newline
    emitter.emit("const char *name = ");
    emitter.emit("\"{}\";\n", "EzPacker");

    // Raw emission: should not prepend indentation
    emitter.emitRaw("#define RAW_MACRO 1\n");

    std::string expected =
            "    int value = 42;\n"
            "    const char *name = \"EzPacker\";\n"
            "#define RAW_MACRO 1\n";

    EXPECT_EQ(emitter.str(), expected);
}

TEST_F(CppSourceEmitterTest, TestEmitEmptyStrings)
{
    CppSourceEmitter emitter;
    emitter.emit("");
    emitter.emitRaw("");
    emitter.emitLines("");
    EXPECT_TRUE(emitter.empty());

    emitter.emitLine("alpha");
    emitter.emitLine(""); // Empty line delegates to emitBlankLine
    emitter.emitLine("beta");

    std::string expected = "alpha\n\nbeta\n";
    EXPECT_EQ(emitter.str(), expected);
}

// ============================================================================
// 7. C++ Idioms & Common Constructs
// ============================================================================

TEST_F(CppSourceEmitterTest, TestCommonConstructs)
{
    CppSourceEmitter emitter;
    emitter.emitPragmaOnce();
    emitter.emitIncludeGuardStart("MY_GUARD_H");
    emitter.emitBanner("TestGenerator", "GENERATED BY EZDSL.");
    emitter.emitInclude("vector", true);
    emitter.emitInclude("MyHeader.h", false);
    emitter.emitComment("Normal single-line comment");
    emitter.emitSectionComment("Core Engine");
    emitter.emitDocComment("Doc comment line 1\nDoc comment line 2");
    emitter.emitIncludeGuardEnd("MY_GUARD_H");

    const std::string &out = emitter.str();
    EXPECT_TRUE(out.find("#pragma once") != std::string::npos);
    EXPECT_TRUE(out.find("#ifndef MY_GUARD_H") != std::string::npos);
    EXPECT_TRUE(out.find("#define MY_GUARD_H") != std::string::npos);
    EXPECT_TRUE(out.find("Auto-generated by EzDSL TestGenerator. GENERATED BY EZDSL.") != std::string::npos);
    EXPECT_TRUE(out.find("#include <vector>") != std::string::npos);
    EXPECT_TRUE(out.find("#include \"MyHeader.h\"") != std::string::npos);
    EXPECT_TRUE(out.find("// Normal single-line comment") != std::string::npos);
    EXPECT_TRUE(out.find("// --- Core Engine ---") != std::string::npos);
    EXPECT_TRUE(out.find("/**\nDoc comment line 1\nDoc comment line 2\n */") != std::string::npos);
    EXPECT_TRUE(out.find("#endif // MY_GUARD_H") != std::string::npos);
}

// ============================================================================
// 8. Buffer State & Lifecycle Operations
// ============================================================================

TEST_F(CppSourceEmitterTest, TestBufferManagementTakeStrAndClear)
{
    CppSourceEmitter emitter;
    emitter.indent();
    emitter.emitLine("content line;");
    EXPECT_FALSE(emitter.empty());
    EXPECT_GT(emitter.size(), 0);
    EXPECT_EQ(emitter.view(), "    content line;\n");

    std::string extracted = emitter.takeStr();
    EXPECT_EQ(extracted, "    content line;\n");
    EXPECT_TRUE(emitter.empty());
    EXPECT_EQ(emitter.size(), 0);
    EXPECT_EQ(emitter.getIndentLevel(), 0); // takeStr resets indent level

    emitter.indent();
    emitter.emitLine("new content;");
    EXPECT_FALSE(emitter.empty());
    emitter.clear();
    EXPECT_TRUE(emitter.empty());
    EXPECT_EQ(emitter.getIndentLevel(), 0);
}
